#include "led.h"

#include <esp_log.h>

#include "components/HeaderBar.h"

CRGB leds[NUM_LEDS];
static auto TAG = "LED";

void initLED() {
    ESP_LOGI(TAG, "Initializing RGB LED on pin %d...", Pins::Display::ledPin);

    FastLED.addLeds<LED_TYPE, Pins::Display::ledPin, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(128);  // Set to 50% brightness by default

    // Turn off LED initially
    setLEDOff();

    ESP_LOGI(TAG, "RGB LED initialization complete.");
}

void setLEDColor(CRGB color) {
    leds[0] = color;
    FastLED.show();
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    leds[0] = CRGB(r, g, b);
    FastLED.show();
}

void setLEDOff() {
    leds[0] = COLOR_OFF;
    FastLED.show();
}

void setLEDBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void flashLED(CRGB color, int duration_ms) {
    setLEDColor(color);
    delay(duration_ms);
    setLEDOff();
}

void breatheLED(CRGB color, int period_ms, int step = 5, int max = 255, int min = 0) {
    static unsigned long lastUpdate = 0;
    static int breatheValue = 0;
    static bool increasing = true;

    unsigned long currentTime = millis();
    if (currentTime - lastUpdate > period_ms) {
        lastUpdate = currentTime;

        if (increasing) {
            breatheValue += step;
            if (breatheValue >= max) {
                increasing = false;
            }
        } else {
            breatheValue -= step;
            if (breatheValue <= min) {
                increasing = true;
            }
        }

        CRGB dimmedColor = color;
        dimmedColor.nscale8(breatheValue);
        setLEDColor(dimmedColor);
    }
}

void cycleLED(int period_ms) {
    static unsigned long lastUpdate = 0;
    static uint8_t hue = 0;

    unsigned long currentTime = millis();
    if (currentTime - lastUpdate > (period_ms / 256)) {
        lastUpdate = currentTime;
        hue++;
        setLEDColor(CHSV(hue, 255, 255));
    }
}

// Status indication functions
void setLEDStatusIdle() {
    setLEDColor(COLOR_STATUS_IDLE);
    ESP_LOGD(TAG, "LED status: IDLE (Blue)");
}

void setLEDStatusRunning() {
    setLEDColor(COLOR_STATUS_RUNNING);
    ESP_LOGD(TAG, "LED status: RUNNING (Green)");
}

void setLEDStatusError() {
    setLEDColor(COLOR_STATUS_ERROR);
    ESP_LOGD(TAG, "LED status: ERROR (Red)");
}

void setLEDStatusHoming() {
    setLEDColor(COLOR_STATUS_HOMING);
    ESP_LOGD(TAG, "LED status: HOMING (Yellow)");
}

void setLEDStatusConnecting() {
    setLEDColor(COLOR_STATUS_CONNECTING);
    ESP_LOGD(TAG, "LED status: CONNECTING (Purple)");
}

// BLE status indication functions
static BleStatus lastBLEStatus = BleStatus::DISCONNECTED;
static unsigned long bleStatusChangeTime = 0;
static unsigned long rainbowStartTime = 0;
static bool rainbowActive = false;
static bool fadeActive = false;
static bool connectedDimmed = false;  // New flag for dimmed connected state
static uint8_t fadeValue = 255;
static int pulseCount = 0;
static bool pulseState = false;

// Communication pulse variables
static bool commPulseActive = false;
static unsigned long commPulseStartTime = 0;
static uint8_t baseDimLevel = 30;  // Dim level when connected (out of 255)

// Advertising timeout variables
static bool advertisingDimmed = false;  // Flag for dimmed advertising after timeout
static unsigned long advertisingStartTime = 0;  // When advertising started
static const unsigned long ADVERTISING_TIMEOUT = 30000;  // 30 seconds in milliseconds

void updateLEDForBLEStatus() {
    BleStatus currentStatus = getBleStatus();
    unsigned long currentTime = millis();

    // Check if BLE status changed
    if (currentStatus != lastBLEStatus) {
        lastBLEStatus = currentStatus;
        bleStatusChangeTime = currentTime;
        rainbowActive = false;
        fadeActive = false;
        connectedDimmed = false;
        pulseCount = 0;
        pulseState = false;

        ESP_LOGI(TAG, "BLE status changed to: %d", (int)currentStatus);

        switch (currentStatus) {
            case BleStatus::ADVERTISING:
                // Rainbow will be handled in the continuous loop
                rainbowActive = true;
                rainbowStartTime = currentTime;
                advertisingStartTime = currentTime; // Track when advertising started
                advertisingDimmed = false;  // Reset dimmed flag
                break;
            case BleStatus::CONNECTED:
                // Rainbow will be handled in the continuous loop
                rainbowActive = true;
                rainbowStartTime = currentTime;
                advertisingDimmed = false;  // Reset advertising flags
                break;
            case BleStatus::DISCONNECTED:
                showBLEDisconnected();
                advertisingDimmed = false;  // Reset advertising flags
                break;
            default:
                break;
        }
    }

    // Handle ongoing effects based on current status
    switch (currentStatus) {
        case BleStatus::ADVERTISING:
            if (rainbowActive) {
                showBLERainbow(1000);
                if (millis() - rainbowStartTime >= 1000) {
                    rainbowActive = false;
                }
            } else {
                // Check if advertising timeout has been reached
                unsigned long advertisingElapsed =
                    currentTime - advertisingStartTime;
                if (advertisingElapsed >= ADVERTISING_TIMEOUT &&
                    !advertisingDimmed) {
                    advertisingDimmed = true;
                    ESP_LOGI(TAG,
                             "Advertising timeout reached, switching to dimmed "
                             "pulsing");
                }

                if (advertisingDimmed) {
                    breatheLED(COLOR_BLUE, 20, 5, 50);
                } else {
                    breatheLED(COLOR_BLUE, 20, 5);
                }
            }
            break;

        case BleStatus::CONNECTED:
            if (rainbowActive) {
                showBLERainbow(1000);
                if (millis() - rainbowStartTime >= 1000) {
                    rainbowActive = false;
                    fadeActive = true;
                    fadeValue = 255;
                }
            } else if (fadeActive) {
                showBLEConnected();
            } else if (connectedDimmed) {
                showBLEConnected();
            } else {
                // Stay dimmed once fade is complete
                connectedDimmed = true;
                showBLEConnected();
            }
            break;

        case BleStatus::DISCONNECTED:
            // LED should be off
            break;

        default:
            break;
    }
}

void showBLERainbow(int duration_ms) {
    if (!rainbowActive) {
        rainbowActive = true;
        rainbowStartTime = millis();
    }

    unsigned long elapsed = millis() - rainbowStartTime;
    if (elapsed < duration_ms && rainbowActive) {
        // Calculate hue based on time elapsed - full rainbow cycle in
        // duration_ms
        uint8_t hue = (elapsed * 255) / duration_ms;
        setLEDColor(CHSV(hue, 255, 255));
    }
}

void showBLEConnected() {
    if (fadeActive) {
        // Fade down to dim level (not all the way to 0)
        unsigned long currentTime = millis();
        static unsigned long lastFadeTime = 0;

        if (currentTime - lastFadeTime >=
            20) {  // Update every 20ms for smooth fade
            lastFadeTime = currentTime;

            if (fadeValue > baseDimLevel) {
                fadeValue -= 2;  // Fade speed
                if (fadeValue <= baseDimLevel) {
                    fadeValue = baseDimLevel;
                    fadeActive = false;
                    connectedDimmed = true;
                }
            }

            // Handle communication pulse overlay
            uint8_t displayValue = fadeValue;
            if (commPulseActive) {
                unsigned long pulseElapsed = currentTime - commPulseStartTime;
                if (pulseElapsed < 100) {  // 100ms pulse duration (shorter)
                    // Subtle increase brightness for communication pulse
                    uint8_t pulseBoost =
                        map(pulseElapsed, 0, 100, 15,
                            0);  // Fade from +15 to +0 (much more subtle)
                    displayValue = min(255, fadeValue + pulseBoost);
                } else {
                    commPulseActive = false;
                }
            }

            CRGB color = CRGB::Blue;
            color.nscale8(displayValue);
            setLEDColor(color);
        }
    } else if (connectedDimmed) {
        // Stay at dim level, but handle communication pulses
        unsigned long currentTime = millis();
        uint8_t displayValue = baseDimLevel;

        if (commPulseActive) {
            unsigned long pulseElapsed = currentTime - commPulseStartTime;
            if (pulseElapsed < 100) {  // 100ms pulse duration (shorter)
                // Subtle increase brightness for communication pulse
                uint8_t pulseBoost =
                    map(pulseElapsed, 0, 100, 15,
                        0);  // Fade from +15 to +0 (much more subtle)
                displayValue = min(255, baseDimLevel + pulseBoost);
            } else {
                commPulseActive = false;
            }
        }

        CRGB color = CRGB::Blue;
        color.nscale8(displayValue);
        setLEDColor(color);
    }
}

void showBLEDisconnected() {
    setLEDOff();
    ESP_LOGD(TAG, "BLE LED status: DISCONNECTED (Off)");
}

// Communication pulse functions
void pulseForCommunication() {
    // Only pulse if we're in connected dimmed state
    if (connectedDimmed) {
        commPulseActive = true;
        commPulseStartTime = millis();
    }
}

// Machine status functions
static bool homingActiveFlag = false;

void setHomingActive(bool active) {
    if (homingActiveFlag != active) {
        homingActiveFlag = active;
        ESP_LOGI(TAG, "Homing status changed: %s",
                 active ? "ACTIVE" : "INACTIVE");
    }
}

static bool updateActiveFlag = false;

void setUpdateActive(bool active) {
    if (updateActiveFlag != active) {
        updateActiveFlag = active;
        ESP_LOGI(TAG, "Update status changed: %s",
                 active ? "ACTIVE" : "INACTIVE");
    }
}

void updateLEDForMachineStatus() {
    // Check if homing is active - this takes priority over BLE status
    if (homingActiveFlag) {
        breatheLED(COLOR_DEEP_PURPLE, 20, 5);
    } else if(updateActiveFlag) {
        breatheLED(COLOR_YELLOW, 20, 5);
    } else {
        // Fall back to BLE status
        updateLEDForBLEStatus();
    }
}