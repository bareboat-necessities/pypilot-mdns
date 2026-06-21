#include <cassert>
#include <pypilot_mdns.hpp>

int main() {
    pypilot_mdns::PypilotMdns mdns;
    assert(mdns.last_error() != nullptr);
    return 0;
}
