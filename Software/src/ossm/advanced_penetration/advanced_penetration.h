#ifndef OSSM_ADVANCED_PENETRATION_H
#define OSSM_ADVANCED_PENETRATION_H

#include <NimBLECharacteristic.h>
#include <NimBLEService.h>
#include <NimBLEUUID.h>

#include "ossm/state/state.h"

namespace advanced_penetration {
    #define ADVANCED_SERVICE_UUID "4F53534D-6164-7661-6E63-65646D6F6465"
    #define CHARACTERISTIC_ADVANCED_STATUS_UUID "4F53534D-6164-7661-6E63-656473746174"
    #define CHARACTERISTIC_ADVANCED_CONFIG_UUID "4F53534D-6164-7661-6E63-6564636F6E66"
    #define CHARACTERISTIC_ADVANCED_CONTROL_UUID "4F53534D-6164-7661-6E63-6564636F6D6D"

    void advancedClick();

    void startAdvancedPenetration();

    void initNimble();
}

#endif