#include "myWifi.h"

const char* hostName;

const char* getHostname() { return hostName; }


const char* ssid = "bkn";
const char* password = "5d4bf72344";

bool networkUp = false;

// By default 'time.nist.gov' is used with 60 seconds update interval and
// no offset
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

time_t ntpSyncProvider() {
  return timeClient.getEpochTime();
}

void setupWifi(const char* hostname, int32_t timeZoneOffset) {
  hostName = hostname;

  Serial.println("Begining setupWifi()");
  Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
  Serial.printf("Hostname: %s\n", getHostname());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  timeClient.begin();
  timeClient.setTimeOffset(timeZoneOffset);

  setSyncProvider(&ntpSyncProvider);
  setSyncInterval(5);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(getHostname());

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loopWifi() {

  bool wasUp = networkUp;
  networkUp = WiFi.isConnected();

  if (wasUp != networkUp) {
    WiFi.printDiag(Serial);
    if (networkUp) {
      Serial.println("Network up!");
    } else {
      Serial.println("Network Down!");
    }
  }

  ArduinoOTA.handle();
  timeClient.update();
}

int32_t httpGet(const char* url) {

    HTTPClient http;

    http.begin(url); //HTTP

    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
//            String payload = http.getString();
//            Serial.println(payload);
        }
    }
    Serial.printf("[HTTP] GET %s returned: %d , error: %s\n", url, httpCode, http.errorToString(httpCode).c_str());

    http.end();

    return httpCode;
}
