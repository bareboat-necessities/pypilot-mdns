#include <pypilot_mdns.hpp>

#if defined(ARDUINO) && defined(ESP32)

namespace pypilot_mdns {

PypilotMdns::PypilotMdns() = default;

PypilotMdns::~PypilotMdns() {
    end();
}

} // namespace pypilot_mdns

#endif
