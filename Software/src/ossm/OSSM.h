#ifndef OSSM_SOFTWARE_OSSM_H
#define OSSM_SOFTWARE_OSSM_H

#include <Arduino.h>

#include "ossm/state/ble.h"
#include "ossm/state/calibration.h"
#include "ossm/state/menu.h"
#include "ossm/state/settings.h"

/**
 * OSSM class - Minimal backward compatibility shim
 *
 * This class is being deprecated in favor of:
 * - Global state singletons (settings, calibration, bleState, etc.)
 * - Stateless feature modules (pages::, homing::, menu::, etc.)
 *
 * The class is kept temporarily for:
 * - BLE command handling (ble_click)
 * - getCurrentState() for status reporting
 * - Static OSSM::setting for backward compatibility
 */
class OSSM {
  public:
    explicit OSSM();

    // BLE command handler
    void ble_click(String commandString);

    // Get current state as JSON string (includes timestamp)
    String getCurrentState();

    // Get state fingerprint without timestamp (for change detection)
    String getStateFingerprint();
};

extern OSSM *ossm;

#endif  // OSSM_SOFTWARE_OSSM_H
