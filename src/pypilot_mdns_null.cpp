#include <pypilot_mdns.hpp>

#if !defined(ARDUINO) && !defined(PYPILOT_MDNS_HAVE_AVAHI)

namespace pypilot_mdns {

PypilotMdns::PypilotMdns() = default;
PypilotMdns::~PypilotMdns() = default;

bool PypilotMdns::begin(const char* hostname) {
    mdns_copy(hostname_, sizeof(hostname_), hostname ? hostname : "pypilot");
    mdns_copy(last_error_, sizeof(last_error_), "Avahi backend not compiled");
    return false;
}

void PypilotMdns::end() {
}

bool PypilotMdns::advertise_pypilot(uint16_t, const char*, const char*) {
    mdns_copy(last_error_, sizeof(last_error_), "Avahi backend not compiled");
    return false;
}

bool PypilotMdns::discover_signalk(MdnsServiceInfo&, uint32_t) {
    mdns_copy(last_error_, sizeof(last_error_), "Avahi backend not compiled");
    return false;
}

} // namespace pypilot_mdns

#endif
