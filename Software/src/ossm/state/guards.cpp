#include "guards.h"

#include "constants/Pins.h"
#include "ossm/homing/homing.h"
#include "ossm/state/menu.h"
#include "utils/analog.h"

bool ossmIsStrokeTooShort() { return homing::isStrokeTooShort(); }

bool ossmIsNotHomed() { return !calibration.isHomed; }

bool ossmIsPreflightSafe() {
    float potReading = getAnalogAveragePercent({Pins::Remote::speedPotPin, 50});

    return potReading < 1.0; //Moved to static as this is never going to change.
}

Menu ossmGetMenuOption() { return menuState.currentOption; }
