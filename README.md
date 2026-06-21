# pypilot-mdns

Unified mDNS/DNS-SD helper for the modular C++ pypilot port.

The module provides one API for:

- advertising the pypilot TCP service as `_pypilot._tcp.local.`;
- discovering Signal K by browsing `_http._tcp.local.` and filtering TXT records for `swname=signalk-server`;
- Linux builds using Avahi when available;
- Arduino ESP32/ESP32-S3 builds using `ESPmDNS`.

The original Python pypilot behavior is:

```text
pypilot service: _pypilot._tcp.local.
Signal K discovery: browse _http._tcp.local. and filter swname=signalk-server
```

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Linux Avahi support is enabled when `pkg-config` can find `avahi-client` and `avahi-common`. Without Avahi headers, the Linux build falls back to a no-op backend that still lets dependent modules compile.

## Arduino

The Arduino backend is enabled when compiling for ESP32:

```cpp
#include <pypilot_mdns.hpp>

pypilot_mdns::PypilotMdns mdns;
mdns.begin("pypilot");
mdns.advertise_pypilot(23322);
```

Arduino uses `ESPmDNS` and therefore requires WiFi to be connected before calling `begin()`.

## API

```cpp
pypilot_mdns::PypilotMdns mdns;
mdns.begin("pypilot");
mdns.advertise_pypilot(23322);

pypilot_mdns::MdnsServiceInfo signalk;
if (mdns.discover_signalk(signalk, 3000)) {
    // signalk.host, signalk.address, signalk.port
}
```

## Files

```text
src/pypilot_mdns.hpp
src/pypilot_mdns_avahi.cpp
src/pypilot_mdns_null.cpp
tests/test_mdns_types.cpp
tests/test_mdns_compile.cpp
examples/MdnsExample/MdnsExample.cpp
examples/MdnsExample/MdnsExample.ino
```
