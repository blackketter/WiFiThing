#ifndef _WiFiThing_
#define _WiFiThing_

#include <WiFiConsole.h>
#include <Timezone.h>
#include <Timezones.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
extern ESP8266WebServer server;
#endif

#ifdef ESP32
#include <WiFi.h>
#include <mDNS.h>
#include <HTTPClient.h>
#include <WebServer.h>
extern WebServer server;
#endif

class WiFiThing {
  public:
    // call wifithing.begin() in your arduino setup().
    void begin(const char* ssid = nullptr, const char *passphrase = nullptr);

    // call wifithing.idle() in your arduino loop()
    void idle();

    // to override the default hostname
    void setHostname(const char* hostname);

    // to get the hostname
    static String getHostname();
    static String getMacAddress();
    static String getIPAddress();
    // to make an HTTP request, ignoring response, returning response code
    int32_t httpGet(const char* url);

    void setTimezone(Timezone* localZone);
  private:
    void beginServer();
    String _hostname;
};

extern WiFiConsole console;
#endif
