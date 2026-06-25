#ifndef UI_MENU_ITEMS_H
#define UI_MENU_ITEMS_H

#include "Strings.h"

enum Menu {
    AdvancedPenetration,
    StrokeEngine,
    Streaming,
    UpdateOSSM,
    Help,
    WiFiSetup,
    Restart,
    NUM_OPTIONS
};

static const char* menuStrings[Menu::NUM_OPTIONS] = {
    ui::strings::advancedPenetration,
    ui::strings::strokeEngine,
    ui::strings::streaming,
    ui::strings::update,
    ui::strings::wifiSetup,
    ui::strings::helpTitle,
    ui::strings::restart,
};

#endif  // UI_MENU_ITEMS_H
