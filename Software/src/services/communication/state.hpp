#ifndef OSSM_COMMUNICATION_STATE_HPP
#define OSSM_COMMUNICATION_STATE_HPP

#include "Arduino.h"
#include "NimBLECharacteristic.h"
#include "NimBLEService.h"
#include "NimBLEUUID.h"

inline NimBLECharacteristic* initStateCharacteristic(NimBLEService* pService,
                                              NimBLEUUID uuid) {
    // State characteristic (read/notify string payload for state)
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Current state of the device");

    static const char boot_state[] PROGMEM = "ok:boot";
    pChar->setValue(String(FPSTR(boot_state)));

    return pChar;
}

#endif  // OSSM_COMMUNICATION_STATE_HPP
