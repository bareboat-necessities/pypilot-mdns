#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#if defined(ARDUINO) && defined(ESP32)
#include <Arduino.h>
#include <ESPmDNS.h>
#endif

namespace pypilot_mdns {

static constexpr const char* PYPILOT_MDNS_PYPILOT_SERVICE = "pypilot";
static constexpr const char* PYPILOT_MDNS_PYPILOT_PROTO = "tcp";
static constexpr const char* PYPILOT_MDNS_SIGNALK_QUERY_SERVICE = "http";
static constexpr const char* PYPILOT_MDNS_SIGNALK_QUERY_PROTO = "tcp";
static constexpr const char* PYPILOT_MDNS_SIGNALK_SWNAME = "signalk-server";
static constexpr uint16_t PYPILOT_MDNS_DEFAULT_PYPILOT_PORT = 23322;
static constexpr uint16_t PYPILOT_MDNS_DEFAULT_SIGNALK_PORT = 3000;

static inline bool mdns_copy(char* dst, size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) return false;
    if (!src) {
        dst[0] = '\0';
        return true;
    }
    size_t i = 0;
    for (; i + 1 < dst_size && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
    return src[i] == '\0';
}

struct MdnsTxtEntry {
    char key[32]{};
    char value[96]{};
};

template<size_t MaxEntries = 8>
struct MdnsTxtList {
    MdnsTxtEntry entries[MaxEntries]{};
    size_t count = 0;

    bool add(const char* key, const char* value) {
        if (count >= MaxEntries) return false;
        if (!mdns_copy(entries[count].key, sizeof(entries[count].key), key)) return false;
        if (!mdns_copy(entries[count].value, sizeof(entries[count].value), value)) return false;
        ++count;
        return true;
    }

    const char* find(const char* key) const {
        if (!key) return nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (strcmp(entries[i].key, key) == 0) return entries[i].value;
        }
        return nullptr;
    }
};

struct MdnsServiceInfo {
    char instance[96]{};
    char service[32]{};
    char protocol[16]{};
    char host[128]{};
    char address[64]{};
    uint16_t port = 0;
    MdnsTxtList<12> txt;
};

struct PypilotMdnsOptions {
    char hostname[64] = "pypilot";
    char instance[96] = "pypilot";
    uint16_t pypilot_port = PYPILOT_MDNS_DEFAULT_PYPILOT_PORT;
};

#if !defined(ARDUINO)
struct LinuxMdnsState;
#endif

class PypilotMdns {
public:
    PypilotMdns();
    ~PypilotMdns();

#if defined(ARDUINO) && defined(ESP32)
    bool begin(const char* hostname = "pypilot") {
        mdns_copy(hostname_, sizeof(hostname_), hostname ? hostname : "pypilot");
        started_ = MDNS.begin(hostname_);
        if (!started_) mdns_copy(last_error_, sizeof(last_error_), "ESPmDNS begin failed");
        else last_error_[0] = '\0';
        return started_;
    }

    void end() {
        if (started_) MDNS.end();
        started_ = false;
    }

    bool advertise_pypilot(uint16_t port = PYPILOT_MDNS_DEFAULT_PYPILOT_PORT,
                           const char* instance = "pypilot",
                           const char* uid = "pypilot") {
        (void)instance;
        if (!started_) {
            mdns_copy(last_error_, sizeof(last_error_), "mDNS not started");
            return false;
        }
        MDNS.addService(PYPILOT_MDNS_PYPILOT_SERVICE, PYPILOT_MDNS_PYPILOT_PROTO, port);
        MDNS.addServiceTxt(PYPILOT_MDNS_PYPILOT_SERVICE, PYPILOT_MDNS_PYPILOT_PROTO, "swname", "pypilot");
        MDNS.addServiceTxt(PYPILOT_MDNS_PYPILOT_SERVICE, PYPILOT_MDNS_PYPILOT_PROTO, "uid", uid ? uid : "pypilot");
        last_error_[0] = '\0';
        return true;
    }

    bool discover_signalk(MdnsServiceInfo& out, uint32_t timeout_ms = 3000) {
        if (!started_) {
            mdns_copy(last_error_, sizeof(last_error_), "mDNS not started");
            return false;
        }
        const int count = MDNS.queryService(PYPILOT_MDNS_SIGNALK_QUERY_SERVICE, PYPILOT_MDNS_SIGNALK_QUERY_PROTO, timeout_ms);
        for (int i = 0; i < count; ++i) {
            String swname = MDNS.txt(i, "swname");
            if (swname != PYPILOT_MDNS_SIGNALK_SWNAME) continue;
            mdns_copy(out.instance, sizeof(out.instance), MDNS.hostname(i).c_str());
            mdns_copy(out.service, sizeof(out.service), PYPILOT_MDNS_SIGNALK_QUERY_SERVICE);
            mdns_copy(out.protocol, sizeof(out.protocol), PYPILOT_MDNS_SIGNALK_QUERY_PROTO);
            mdns_copy(out.host, sizeof(out.host), MDNS.hostname(i).c_str());
            mdns_copy(out.address, sizeof(out.address), MDNS.IP(i).toString().c_str());
            out.port = static_cast<uint16_t>(MDNS.port(i));
            out.txt.add("swname", swname.c_str());
            last_error_[0] = '\0';
            return true;
        }
        mdns_copy(last_error_, sizeof(last_error_), "Signal K service not found");
        return false;
    }
#else
    bool begin(const char* hostname = "pypilot");
    void end();
    bool advertise_pypilot(uint16_t port = PYPILOT_MDNS_DEFAULT_PYPILOT_PORT,
                           const char* instance = "pypilot",
                           const char* uid = "pypilot");
    bool discover_signalk(MdnsServiceInfo& out, uint32_t timeout_ms = 3000);
#endif

    const char* last_error() const { return last_error_; }

private:
    char hostname_[64] = "pypilot";
    char last_error_[128]{};
#if defined(ARDUINO) && defined(ESP32)
    bool started_ = false;
#else
    LinuxMdnsState* state_ = nullptr;
#endif
};

} // namespace pypilot_mdns
