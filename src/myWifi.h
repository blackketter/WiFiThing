#ifndef _myWifi_
#define _myWifi_

#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

void setupWifi(const char* hostName, int32_t timeZoneOffset = 0);
void loopWifi();

int32_t httpGet(const char* url);

const char* getHostName();

#endif
