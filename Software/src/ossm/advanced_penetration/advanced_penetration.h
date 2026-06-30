#ifndef OSSM_ADVANCED_PENETRATION_H
#define OSSM_ADVANCED_PENETRATION_H

#include <NimBLECharacteristic.h>
#include <NimBLEService.h>
#include <NimBLEUUID.h>

#include "ossm/state/state.h"

namespace advanced_penetration {
#define CHARACTERISTIC_ADVANCED_STATUS_UUID "4F53534D-6164-7661-6E63-656473746174"
#define CHARACTERISTIC_ADVANCED_CONFIG_UUID "4F53534D-6164-7661-6E63-6564636F6E66"
#define CHARACTERISTIC_ADVANCED_CONTROL_UUID "4F53534D-6164-7661-6E63-6564636F6E74"
#define CHARACTERISTIC_ADVANCED_PRESETS_UUID "4F53534D-6164-7661-6E63-656470727374"

    void advancedClick();

    void startAdvancedPenetration();

    void initNimble(NimBLEService* service);
}

#endif