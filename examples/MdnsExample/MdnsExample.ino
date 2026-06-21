#include "MdnsExampleCore.hpp"

void setup() {
    Serial.begin(115200);
    delay(250);
    run_mdns_example_once();
}

void loop() {
}
