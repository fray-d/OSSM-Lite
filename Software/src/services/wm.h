#ifndef OSSM_WM_H
#define OSSM_WM_H

#include "WiFiManager.h"
#include "Arduino.h"

extern WiFiManager wm;

void initWM();
bool connectWiFi(const String& ssid, const String& password);
String getWiFiStatus();

#endif  // OSSM_WM_H
