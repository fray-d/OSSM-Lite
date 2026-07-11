#ifndef OSSM_SOFTWARE_LED_H
#define OSSM_SOFTWARE_LED_H

#include <Arduino.h>
#include <FastLED.h>
#include "constants/Pins.h"

// LED configuration
#define NUM_LEDS 1
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Common colors
#define COLOR_OFF CRGB::Black
#define COLOR_RED CRGB::Red
#define COLOR_GREEN CRGB::Green
#define COLOR_BLUE CRGB::Blue
#define COLOR_WHITE CRGB::White
#define COLOR_YELLOW CRGB::Yellow
#define COLOR_PURPLE CRGB::Purple
#define COLOR_ORANGE CRGB::Orange
#define COLOR_CYAN CRGB::Cyan

// Special effect colors
#define COLOR_DEEP_PURPLE CRGB(75, 0, 130)  // Deep purple for homing breathing

extern CRGB leds[NUM_LEDS];

// LED service functions
void initLED();
void setLEDColor(CRGB color);
void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
void setLEDOff();
void setLEDBrightness(uint8_t brightness);
void breatheLED(CRGB color, int step, int max, int min);

// BLE status indication functions
void updateLEDForBLEStatus();
void showBLERainbow(int duration_ms = 1000);

// Communication pulse functions
void pulseForCommunication();

void updateLEDForMachineStatus();

#endif // OSSM_SOFTWARE_LED_H