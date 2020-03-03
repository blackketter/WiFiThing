#include <Arduino.h>

#include "WiFiThing.h"
// create Credentials.h and define const char* ssid and passphrase
#include "Credentials.h"


WiFiThing thing;
// WiFiConsole console is provided by WiFiThing


void setup() {
  thing.begin(ssid, passphrase);
}


void loop(void) {
  thing.idle();
}
