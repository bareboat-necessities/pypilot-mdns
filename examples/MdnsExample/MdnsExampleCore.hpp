#pragma once

#include <pypilot_mdns.hpp>

#if defined(ARDUINO)
#include <Arduino.h>
static inline void mdns_example_print(const char* text) { Serial.println(text); }
#else
#include <stdio.h>
static inline void mdns_example_print(const char* text) { printf("%s\n", text); }
#endif

static inline void run_mdns_example_once() {
    pypilot_mdns::PypilotMdns mdns;
    if (!mdns.begin("pypilot")) {
        mdns_example_print(mdns.last_error());
        return;
    }
    if (!mdns.advertise_pypilot(pypilot_mdns::PYPILOT_MDNS_DEFAULT_PYPILOT_PORT)) {
        mdns_example_print(mdns.last_error());
        mdns.end();
        return;
    }
    mdns_example_print("advertising _pypilot._tcp.local.");

    pypilot_mdns::MdnsServiceInfo signalk;
    if (mdns.discover_signalk(signalk, 1000)) {
        char line[160]{};
        snprintf(line, sizeof(line), "signalk %s:%u", signalk.address, static_cast<unsigned>(signalk.port));
        mdns_example_print(line);
    } else {
        mdns_example_print(mdns.last_error());
    }
    mdns.end();
}
