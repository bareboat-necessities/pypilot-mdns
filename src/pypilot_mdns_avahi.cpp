#include <pypilot_mdns.hpp>

#if !defined(ARDUINO) && defined(PYPILOT_MDNS_HAVE_AVAHI)

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>
#include <avahi-common/address.h>
#include <avahi-common/error.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>
#include <string.h>
#include <stdlib.h>

namespace pypilot_mdns {

struct LinuxMdnsState {
    AvahiSimplePoll* poll = nullptr;
    AvahiClient* client = nullptr;
    AvahiEntryGroup* group = nullptr;
    bool client_ready = false;
    bool failed = false;
    bool found = false;
    MdnsServiceInfo* discovery = nullptr;
};

static void client_callback(AvahiClient* client, AvahiClientState state, void* userdata) {
    LinuxMdnsState* s = static_cast<LinuxMdnsState*>(userdata);
    if (!s) return;
    if (state == AVAHI_CLIENT_S_RUNNING) s->client_ready = true;
    if (state == AVAHI_CLIENT_FAILURE) s->failed = true;
    if (client) s->client = client;
}

static bool wait_client_ready(LinuxMdnsState* s, uint32_t timeout_ms) {
    if (!s || !s->poll) return false;
    uint32_t elapsed = 0;
    while (!s->client_ready && !s->failed && elapsed < timeout_ms) {
        avahi_simple_poll_iterate(s->poll, 50);
        elapsed += 50;
    }
    return s->client_ready && !s->failed;
}

static void entry_group_callback(AvahiEntryGroup*, AvahiEntryGroupState state, void* userdata) {
    LinuxMdnsState* s = static_cast<LinuxMdnsState*>(userdata);
    if (!s) return;
    if (state == AVAHI_ENTRY_GROUP_FAILURE || state == AVAHI_ENTRY_GROUP_COLLISION) s->failed = true;
}

PypilotMdns::PypilotMdns() = default;

PypilotMdns::~PypilotMdns() {
    end();
}

bool PypilotMdns::begin(const char* hostname) {
    mdns_copy(hostname_, sizeof(hostname_), hostname ? hostname : "pypilot");
    end();
    state_ = new LinuxMdnsState();
    state_->poll = avahi_simple_poll_new();
    if (!state_->poll) {
        mdns_copy(last_error_, sizeof(last_error_), "avahi_simple_poll_new failed");
        end();
        return false;
    }
    int error = 0;
    state_->client = avahi_client_new(avahi_simple_poll_get(state_->poll), AVAHI_CLIENT_NO_FAIL, client_callback, state_, &error);
    if (!state_->client) {
        snprintf(last_error_, sizeof(last_error_), "avahi_client_new failed: %s", avahi_strerror(error));
        end();
        return false;
    }
    if (!wait_client_ready(state_, 3000)) {
        mdns_copy(last_error_, sizeof(last_error_), "Avahi client not ready");
        return false;
    }
    last_error_[0] = '\0';
    return true;
}

void PypilotMdns::end() {
    if (!state_) return;
    if (state_->group) avahi_entry_group_free(state_->group);
    if (state_->client) avahi_client_free(state_->client);
    if (state_->poll) avahi_simple_poll_free(state_->poll);
    delete state_;
    state_ = nullptr;
}

bool PypilotMdns::advertise_pypilot(uint16_t port, const char* instance, const char* uid) {
    if (!state_ && !begin(hostname_)) return false;
    if (!wait_client_ready(state_, 3000)) {
        mdns_copy(last_error_, sizeof(last_error_), "Avahi client not ready");
        return false;
    }
    if (!state_->group) {
        state_->group = avahi_entry_group_new(state_->client, entry_group_callback, state_);
        if (!state_->group) {
            mdns_copy(last_error_, sizeof(last_error_), "avahi_entry_group_new failed");
            return false;
        }
    }
    avahi_entry_group_reset(state_->group);
    const char* service_instance = instance && *instance ? instance : "pypilot";
    int ret = avahi_entry_group_add_service(state_->group,
                                            AVAHI_IF_UNSPEC,
                                            AVAHI_PROTO_UNSPEC,
                                            static_cast<AvahiPublishFlags>(0),
                                            service_instance,
                                            "_pypilot._tcp",
                                            nullptr,
                                            nullptr,
                                            port,
                                            "swname=pypilot",
                                            uid && *uid ? "uid=pypilot" : "uid=pypilot",
                                            nullptr);
    if (ret < 0) {
        snprintf(last_error_, sizeof(last_error_), "avahi_entry_group_add_service failed: %s", avahi_strerror(ret));
        return false;
    }
    ret = avahi_entry_group_commit(state_->group);
    if (ret < 0) {
        snprintf(last_error_, sizeof(last_error_), "avahi_entry_group_commit failed: %s", avahi_strerror(ret));
        return false;
    }
    last_error_[0] = '\0';
    return true;
}

static void resolver_callback(AvahiServiceResolver* resolver,
                              AvahiIfIndex,
                              AvahiProtocol,
                              AvahiResolverEvent event,
                              const char* name,
                              const char* type,
                              const char*,
                              const char* host_name,
                              const AvahiAddress* address,
                              uint16_t port,
                              AvahiStringList* txt,
                              AvahiLookupResultFlags,
                              void* userdata) {
    LinuxMdnsState* s = static_cast<LinuxMdnsState*>(userdata);
    if (!s) return;
    if (event == AVAHI_RESOLVER_FOUND && s->discovery) {
        char swname[128]{};
        char addr[AVAHI_ADDRESS_STR_MAX]{};
        avahi_address_snprint(addr, sizeof(addr), address);
        AvahiStringList* item = avahi_string_list_find(txt, "swname");
        if (item) {
            char* key = nullptr;
            char* value = nullptr;
            size_t size = 0;
            if (avahi_string_list_get_pair(item, &key, &value, &size) == 0) {
                if (value) mdns_copy(swname, sizeof(swname), value);
                if (key) avahi_free(key);
                if (value) avahi_free(value);
            }
        }
        if (strcmp(swname, PYPILOT_MDNS_SIGNALK_SWNAME) == 0) {
            mdns_copy(s->discovery->instance, sizeof(s->discovery->instance), name);
            mdns_copy(s->discovery->service, sizeof(s->discovery->service), type ? type : "_http._tcp");
            mdns_copy(s->discovery->protocol, sizeof(s->discovery->protocol), "tcp");
            mdns_copy(s->discovery->host, sizeof(s->discovery->host), host_name);
            mdns_copy(s->discovery->address, sizeof(s->discovery->address), addr);
            s->discovery->port = port;
            s->discovery->txt.add("swname", swname);
            s->found = true;
            avahi_simple_poll_quit(s->poll);
        }
    }
    if (resolver) avahi_service_resolver_free(resolver);
}

static void browser_callback(AvahiServiceBrowser* browser,
                             AvahiIfIndex interface,
                             AvahiProtocol protocol,
                             AvahiBrowserEvent event,
                             const char* name,
                             const char* type,
                             const char* domain,
                             AvahiLookupResultFlags,
                             void* userdata) {
    LinuxMdnsState* s = static_cast<LinuxMdnsState*>(userdata);
    if (!s || event != AVAHI_BROWSER_NEW) return;
    avahi_service_resolver_new(s->client,
                               interface,
                               protocol,
                               name,
                               type,
                               domain,
                               AVAHI_PROTO_UNSPEC,
                               static_cast<AvahiLookupFlags>(0),
                               resolver_callback,
                               s);
    (void)browser;
}

bool PypilotMdns::discover_signalk(MdnsServiceInfo& out, uint32_t timeout_ms) {
    if (!state_ && !begin(hostname_)) return false;
    if (!wait_client_ready(state_, 3000)) {
        mdns_copy(last_error_, sizeof(last_error_), "Avahi client not ready");
        return false;
    }
    state_->found = false;
    state_->discovery = &out;
    AvahiServiceBrowser* browser = avahi_service_browser_new(state_->client,
                                                              AVAHI_IF_UNSPEC,
                                                              AVAHI_PROTO_UNSPEC,
                                                              "_http._tcp",
                                                              nullptr,
                                                              static_cast<AvahiLookupFlags>(0),
                                                              browser_callback,
                                                              state_);
    if (!browser) {
        mdns_copy(last_error_, sizeof(last_error_), "avahi_service_browser_new failed");
        return false;
    }
    uint32_t elapsed = 0;
    while (!state_->found && elapsed < timeout_ms) {
        avahi_simple_poll_iterate(state_->poll, 50);
        elapsed += 50;
    }
    avahi_service_browser_free(browser);
    state_->discovery = nullptr;
    if (!state_->found) {
        mdns_copy(last_error_, sizeof(last_error_), "Signal K service not found");
        return false;
    }
    last_error_[0] = '\0';
    return true;
}

} // namespace pypilot_mdns

#endif
