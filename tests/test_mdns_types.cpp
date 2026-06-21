#include <cassert>
#include <cstring>

#include <pypilot_mdns.hpp>

int main() {
    using namespace pypilot_mdns;
    MdnsTxtList<2> txt;
    assert(txt.add("swname", "signalk-server"));
    assert(txt.add("uid", "pypilot"));
    assert(!txt.add("extra", "overflow"));
    assert(std::strcmp(txt.find("swname"), "signalk-server") == 0);
    assert(std::strcmp(txt.find("uid"), "pypilot") == 0);
    assert(txt.find("missing") == nullptr);

    MdnsServiceInfo info;
    mdns_copy(info.service, sizeof(info.service), PYPILOT_MDNS_SIGNALK_QUERY_SERVICE);
    mdns_copy(info.protocol, sizeof(info.protocol), PYPILOT_MDNS_SIGNALK_QUERY_PROTO);
    info.port = PYPILOT_MDNS_DEFAULT_SIGNALK_PORT;
    assert(std::strcmp(info.service, "http") == 0);
    assert(std::strcmp(info.protocol, "tcp") == 0);
    assert(info.port == 3000);
    return 0;
}
