#ifndef OSSM_STATE_MENU_H
#define OSSM_STATE_MENU_H

#include "MenuItems.h"

/**
 * Menu state - tracks current menu selection
 */
struct MenuState {
    Menu currentOption = Menu::AdvancedPenetration;
};

extern MenuState menuState;

#endif  // OSSM_STATE_MENU_H
