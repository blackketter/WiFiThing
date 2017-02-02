#ifndef _WiFiThing_
#define _WiFiThing_

#include <WiFiConsole.h>

#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WebServer.h>
extern ESP8266WebServer server;
#endif

class WiFiThing {
  public:
    // call wifithing.begin() in your arduino setup().
    void begin(const char* ssid = nullptr, const char *passphrase = nullptr);

    // call wifithing.loop() in your arduino loop()
    void loop();

    // to override the default hostname
    void setHostname(const char* hostname);

    // to get the hostname
    static String getHostname();
    static String getMacAddress();
    static String getIPAddress();
    // to make an HTTP request, ignoring response, returning response code
    int32_t httpGet(const char* url);
  private:
    void beginServer();
};

extern WiFiConsole console;
#endif
