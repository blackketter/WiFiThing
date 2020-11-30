#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <Clock.h>


#include "WiFiThing.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#include "esp_wifi.h"
#endif

String WiFiThing::_hostname;

void resetWatchdog();
void beginWatchdog();
const uint32_t watchdogTimeout = 5;  // seconds

// By default 'time.nist.gov' is used with 60 seconds update interval and
// no offset
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

WiFiConsole console;

Clock ntpClock;

bool networkUp = false;

// reboot command
class RebootCommand : public Command {
  public:
    const char* getName() { return "reboot"; }
    const char* getHelp() { return "Reboot system"; }
    void execute(Console* c, uint8_t paramCount, char** params) {
      c->println("Rebooting now...");
      console.close();
      delay(1000);
      WiFiThing::reboot();
    }
};
RebootCommand theRebootCommand;

// wifi diags command
class WiFiCommand : public Command {
  public:
    const char* getName() { return "wifi"; }
    const char* getHelp() { return "Print WiFi Info"; }
    void execute(Console* c, uint8_t paramCount, char** params) {
      c->println("----------------------------------");
      c->println("WiFi Info");
      WiFi.printDiag(*c);
      c->printf("  Hostname:    %s.local\n", ArduinoOTA.getHostname().c_str());
      c->printf("  MAC Address: %s\n", WiFi.macAddress().c_str());
      c->printf("  IP Address:  %s\n", WiFi.localIP().toString().c_str());
      c->printf("  WiFi:        %s\n", WiFi.isConnected() ? "connected" : "disconnected");
      if (WiFi.isConnected()) {
        String bssid = WiFi.BSSIDstr();
        c->printf("  BSSID:       %s\n", bssid.c_str());
      }
      c->println("----------------------------------");
    }
};
WiFiCommand theWiFiCommand;

// exit console command
class ExitCommand : public Command {
  public:
    const char* getName() { return "exit"; }
    const char* getHelp() { return "Close console connection"; }
    void execute(Console* c, uint8_t paramCount, char** params) {
      c->println("Goodbye!");
      console.close();
    }
};
ExitCommand theExitCommand;


void ntpUpdateCallback(NTPClient* n) {
  int64_t now = n->getEpochMillis();

  Timezone* oldZone = ntpClock.getZone();

  ntpClock.setZone(&UTC);

#if CALCULATE_DRIFT
  static int32_t driftSum = 0;
  static int32_t driftSamples = 0;
  driftSamples++;
  if (driftSamples > 1) {
    int32_t delta = (long)(now - ntpClock.getMillis());
    driftSum += delta;
    console.debugf("time updated by %d millis, %dms drift in %d minutes\n", delta, driftSum, driftSamples );
  }
#endif

  ntpClock.setMillis(now);
  ntpClock.setZone(oldZone);

  timeClient.setRetryInterval(60000);  // TODO: in the case of future DNS failure, which blocks for 5 seconds

}

void WiFiThing::setHostname(const char* hostname) {
  if (hostname) {
    ArduinoOTA.setHostname(hostname);

#if defined(ESP32)
    WiFi.setHostname(hostname);
#else
    WiFi.hostname(hostname);
#endif

    _hostname = hostname;
  }
  console.debugf("Hostname: %s\n", getHostname().c_str());
}

void WiFiThing::begin(const char* ssid, const char *passphrase) {
  console.begin();
  console.debugln("Begining setupWifi()");

  console.debugf("MAC address: %s\n", getMacAddress().c_str());

#if defined(ESP32)
  // turn off power conservation
  esp_wifi_set_ps (WIFI_PS_NONE);
#endif

  WiFi.mode(WIFI_STA);

#if defined(ESP32)
  WiFi.setSleep(false);
#endif

  if (ssid != nullptr) {
    WiFi.begin(ssid, passphrase);
  } else {
    WiFi.begin();
  }

  timeClient.setUpdateCallback(ntpUpdateCallback);
  timeClient.begin();


  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  // Port defaults to 8266 for esp8266, 3232 for esp32
  // ArduinoOTA.setPort(8266);
  // ArduinoOTA.setPort(3232);

  if (_hostname.length()) {
    ArduinoOTA.setHostname(_hostname.c_str());
  }

  ArduinoOTA.onStart([]() {
#if defined(ESP32)
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    console.debugf("Start updating %s\n", type.c_str());

#else
    console.debugln("Start");
#endif
  });

  ArduinoOTA.onEnd([]() {
    console.debugln("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    console.debugf("Progress: %u%%\r", (progress / (total / 100)));
    resetWatchdog();
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

  beginServer();
  beginWatchdog();
}

void WiFiThing::idle() {
  millis_t nowIdle = Uptime::millis();

  if (nowIdle - _lastIdle < _minIdle) {
      delay(10);  // TODO: if I don't include this, the wifi disconnects.  delay(1) doesn't work, nor does yield()  wierd
  }
  _lastIdle = nowIdle;

  console.idle();

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
  server.handleClient();
  resetWatchdog();
}

void resetWatchdog() {
#ifdef ESP32
  esp_task_wdt_reset();
#endif
}

void beginWatchdog() {
#ifdef ESP32
  esp_task_wdt_init(watchdogTimeout, true);

  esp_err_t err = esp_task_wdt_add(NULL);
  switch (err) {
    case ESP_OK:
      console.debugf("Watchdog activated OK (timeout is %d seconds)\n", watchdogTimeout);
      break;
    case ESP_ERR_INVALID_ARG:
      console.debugln("Watchdog activation error: invalid argument");
      break;
    case ESP_ERR_NO_MEM:
      console.debugln("Watchdog activation error: insufficent memory");
      break;
    case ESP_ERR_INVALID_STATE:
      console.debugln("Watchdog activation error: not initialized yet");
      break;
    default:
      console.debugf("Watchdog activation error: %d\n", err);
      break;
  }
#endif
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
  return _hostname;
}

String WiFiThing::getMacAddress() {
  WiFi.mode(WIFI_STA);
  return WiFi.macAddress();
}

String WiFiThing::getIPAddress() {
  return WiFi.localIP().toString();
}

void WiFiThing::setTimezone(Timezone* local) {
  LocalTime::setSystemTimezone(local);
  ntpClock.setZone(local);
}

void WiFiThing::reboot() {
#if defined(ESP32) || defined(ESP8266)
  ESP.restart();
#endif
};

