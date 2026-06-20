#ifndef UI_MENU_ITEMS_H
#define UI_MENU_ITEMS_H

#include "Strings.h"

enum Menu {
    SimplePenetration,
    AdvancedPenetration,
    StrokeEngine,
    Streaming,
    Pairing,
    UpdateOSSM,
    Help,
    WiFiSetup,
    Restart,
    NUM_OPTIONS
};

static const char* menuStrings[Menu::NUM_OPTIONS] = {
    ui::strings::simplePenetration,
    ui::strings::advancedPenetration,
    ui::strings::strokeEngine,
    ui::strings::streaming,
    ui::strings::pairing,
    ui::strings::update,
    ui::strings::helpTitle,
    ui::strings::wifiSetup,
    ui::strings::restart,
};

#endif  // UI_MENU_ITEMS_H
