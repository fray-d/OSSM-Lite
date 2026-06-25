#ifndef UI_MENU_ITEMS_H
#define UI_MENU_ITEMS_H

#include "Strings.h"

enum Menu {
    AdvancedPenetration,
    StrokeEngine,
    Streaming,
    Help,
    WiFiSetup,
    UpdateOSSM,
    Restart,
    NUM_OPTIONS
};

static const char* menuStrings[Menu::NUM_OPTIONS] = {
    ui::strings::advancedPenetration,
    ui::strings::strokeEngine,
    ui::strings::streaming,
    ui::strings::helpTitle,
    ui::strings::wifiSetup,
    ui::strings::update,
    ui::strings::restart,
};

#endif  // UI_MENU_ITEMS_H
