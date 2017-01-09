#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <mDNS.h>
#include <HTTPClient.h>
#endif

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Clock.h>

#include "WiFiThing.h"

// By default 'time.nist.gov' is used with 60 seconds update interval and
// no offset
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiConsole console;

Clock ntpClock;

bool networkUp = false;

class InfoCommand : public Command {
  public:
    InfoCommand() { setName("info"); setHelp("Print System Info"); }
    void execute(Stream* c, uint8_t paramCount, char** params) {
      c->println("----------------------------------");
      c->println("System Info:");
      c->printf("  wifi:        %s\n", WiFi.isConnected() ? "connected" : "disconnected");
      c->printf("  hostname:    %s\n", ArduinoOTA.getHostname().c_str());
      c->printf("  mac address: %s\n", WiFi.macAddress().c_str());
      c->printf("  ip address:  %s\n", WiFi.localIP().toString().c_str());
      c->printf("  date:        %02d:%02d:%02d %04d-%02d-%02d\n", ntpClock.hour(), ntpClock.minute(), ntpClock.second(), ntpClock.year(), ntpClock.month(), ntpClock.day());
      c->printf("  uptime:      %d\n", millis()/1000);
      c->println("----------------------------------");
    }
};

class WiFiCommand : public Command {
  public:
    WiFiCommand() { setName("wifi"); setHelp("Print WiFi Info"); }
    void execute(Stream* c, uint8_t paramCount, char** params) {
      c->println("----------------------------------");
      c->println("WiFi Info");
      WiFi.printDiag(*c);
      c->println("----------------------------------");
    }
};

void ntpUpdateCallback(NTPClient* n) {
  ntpClock.setMillis(n->getEpochMillis());
}

void WiFiThing::setHostname(const char* hostname) {
  if (hostname) {
    ArduinoOTA.setHostname(hostname);
  }
  console.debugf("Hostname: %s\n", getHostname().c_str());
}

void WiFiThing::begin(const char* ssid, const char *passphrase) {
  console.begin();
  console.addCommand(new InfoCommand());
  console.addCommand(new WiFiCommand());
  console.debugln("Begining setupWifi()");
  console.debugf("MAC address: %s\n", getMacAddress().c_str());

  WiFi.mode(WIFI_STA);

  if (ssid != nullptr) {
    WiFi.begin(ssid, passphrase);
  } else {
    WiFi.begin();
  }

  timeClient.setUpdateCallback(ntpUpdateCallback);
  timeClient.begin();


  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  ArduinoOTA.onStart([]() {
    console.debugln("Start");
  });
  ArduinoOTA.onEnd([]() {
    console.debugln("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    console.debugf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    console.debugf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) console.debugln("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) console.debugln("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) console.debugln("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) console.debugln("Receive Failed");
    else if (error == OTA_END_ERROR) console.debugln("End Failed");
  });
  ArduinoOTA.begin();
}

void WiFiThing::loop() {

  console.loop();

  bool wasUp = networkUp;
  networkUp = WiFi.isConnected();

  if (wasUp != networkUp) {
    if (networkUp) {
      console.debugln("Network up!");
    } else {
      console.debugln("Network Down!");
    }
  }

  if (networkUp) {
    ArduinoOTA.handle();
    timeClient.update();
  }
}

int32_t WiFiThing::httpGet(const char* url) {

    HTTPClient http;

    http.begin(url); //HTTP

    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
//            String payload = http.getString();
//            console.println(payload);
        }
    }
    console.debugf("[HTTP] GET %s returned: %d , error: %s\n", url, httpCode, http.errorToString(httpCode).c_str());

    http.end();

    return httpCode;
}

String WiFiThing::getHostname() {
  return ArduinoOTA.getHostname();
}

String WiFiThing::getMacAddress() {
  return WiFi.macAddress();
}

String WiFiThing::getIPAddress() {
  return WiFi.localIP().toString();
}