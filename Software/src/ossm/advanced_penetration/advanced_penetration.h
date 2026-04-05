#ifndef OSSM_ADVANCED_PENETRATION_H
#define OSSM_ADVANCED_PENETRATION_H

#include <NimBLECharacteristic.h>
#include <NimBLEService.h>
#include <NimBLEUUID.h>

#include "ossm/state/state.h"

namespace advanced_penetration {

    void advancedClick();

    void startAdvancedPenetration();

    NimBLECharacteristic* initAdvancedCommandCharacteristic(NimBLEService* pService, NimBLEUUID uuid);
    NimBLECharacteristic* initAdvancedConfigCharacteristic(NimBLEService* pService, NimBLEUUID uuid);
    NimBLECharacteristic* initAdvancedStatusCharacteristic(NimBLEService* pService, NimBLEUUID uuid);
}

#endif
