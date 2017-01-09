#ifndef _WiFiThing_
#define _WiFiThing_

#include <WiFiConsole.h>

class WiFiThing {
  public:
    // call wifithing.begin() in your arduino setup().
    void begin(const char* ssid = nullptr, const char *passphrase = nullptr);

    // call wifithing.loop() in your arduino loop()
    void loop();

    // to override the default hostname
    void setHostname(const char* hostname);

    // to get the hostname
    String getHostname();
    String getMacAddress();
    String getIPAddress();
    // to make an HTTP request, ignoring response, returning response code
    int32_t httpGet(const char* url);
};

extern WiFiConsole console;

#endif
