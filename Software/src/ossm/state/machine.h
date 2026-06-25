#ifndef OSSM_STATE_MACHINE_H
#define OSSM_STATE_MACHINE_H

#include "../../utils/update.h"
#include "../Events.h"
#include "actions.h"
#include "boost/sml.hpp"
#include "guards.h"

namespace sml = boost::sml;

struct OSSMStateMachine {
    auto operator()() const {
        using namespace sml;
        using namespace actions;
        using namespace guards;

        return make_transition_table(
            // clang-format off
            *"idle"_s + done / drawHello = "homing"_s,

            "homing"_s / startHoming = "homing.run"_s,
            "homing.run"_s + error = "error"_s,
            "homing.run"_s + done[isFirstHomed] / startHoming = "measure.run"_s,
            "homing.run"_s + done[(isOption(Menu::AdvancedPenetration))] / setHomed = "advancedPenetration"_s,
            "homing.run"_s + done[(isOption(Menu::StrokeEngine))] / setHomed = "strokeEngine"_s,
            "homing.run"_s + done[(isOption(Menu::Streaming))] / setHomed = "streaming"_s,
            "homing.run"_s + done / setHomed = "menu"_s,
            "measure.run"_s + done[(isStrokeTooShort)] = "error"_s,
            "measure.run"_s + done / setHomed = "menu"_s,

            "menu"_s / (drawMenu) = "menu.idle"_s,
            "menu.idle"_s + buttonPress[(isOption(Menu::StrokeEngine))] = "strokeEngine"_s,
            "menu.idle"_s + buttonPress[(isOption(Menu::Streaming))] = "streaming"_s,
            "menu.idle"_s + buttonPress[(isOption(Menu::AdvancedPenetration))] = "advancedPenetration"_s,
            "menu.idle"_s + buttonPress[(isOption(Menu::UpdateOSSM))] = "update"_s,
            "menu.idle"_s + buttonPress[(isOption(Menu::WiFiSetup))] = "wifi"_s,
            "menu.idle"_s + buttonPress[isOption(Menu::Help)] = "help"_s,
            "menu.idle"_s + buttonPress[(isOption(Menu::Restart))] = "restart"_s,

            "advancedPenetration"_s [isNotHomed] = "homing"_s,
            "advancedPenetration"_s [isPreflightSafe] / (startAdvancedPenetration) = "advancedPenetration.idle"_s,
            "advancedPenetration"_s / drawPreflight = "advancedPenetration.preflight"_s,
            "advancedPenetration.preflight"_s + done / (startAdvancedPenetration) = "advancedPenetration.idle"_s,
            "advancedPenetration.preflight"_s + longPress / emergencyStop = "menu"_s,
            "advancedPenetration.idle"_s + longPress / emergencyStop = "menu"_s,
            "advancedPenetration.idle"_s + buttonPress / advancedClick = "advancedPenetration.idle"_s,
            "advancedPenetration.idle"_s + doublePress = "advancedPenetration.presets"_s,
            "advancedPenetration.idle"_s + event<ReturnToMenu> / emergencyStop = "menu"_s,
            "advancedPenetration.presets"_s + longPress / emergencyStop = "menu"_s,
            "advancedPenetration.presets"_s + buttonPress / advancedClick = "advancedPenetration.presets"_s,
            "advancedPenetration.presets"_s + done = "advancedPenetration.idle"_s,
            "advancedPenetration.presets"_s + doublePress / advancedClick = "advancedPenetration.idle"_s,

            "strokeEngine"_s [isNotHomed] = "homing"_s,
            "strokeEngine"_s [isPreflightSafe] / (resetSettingsStrokeEngine, drawPlayControls, startStrokeEngine) = "strokeEngine.idle"_s,
            "strokeEngine"_s / drawPreflight = "strokeEngine.preflight"_s,
            "strokeEngine.preflight"_s + done / (resetSettingsStrokeEngine, drawPlayControls, startStrokeEngine) = "strokeEngine.idle"_s,
            "strokeEngine.preflight"_s + longPress / emergencyStop = "menu"_s,
            "strokeEngine.idle"_s + buttonPress / incrementControlStrokeEngine = "strokeEngine.idle"_s,
            "strokeEngine.idle"_s + doublePress / drawPatternControls = "strokeEngine.pattern"_s,
            "strokeEngine.pattern"_s + buttonPress / drawPlayControls = "strokeEngine.idle"_s,
            "strokeEngine.pattern"_s + doublePress / drawPlayControls = "strokeEngine.idle"_s,
            "strokeEngine.pattern"_s + longPress / emergencyStop = "menu"_s,
            "strokeEngine.idle"_s + longPress / emergencyStop = "menu"_s,
            "strokeEngine.idle"_s + event<ReturnToMenu> / emergencyStop = "menu"_s,
            "strokeEngine.pattern"_s + event<ReturnToMenu> / emergencyStop = "menu"_s,
            "strokeEngine.preflight"_s + event<ReturnToMenu> / emergencyStop = "menu"_s,

            "streaming"_s [isNotHomed] = "homing"_s,
            "streaming"_s [isPreflightSafe] / (resetSettingsStreaming, drawPlayControls, startStreaming) = "streaming.idle"_s,
            "streaming"_s / drawPreflight = "streaming.preflight"_s,
            "streaming.preflight"_s + done / (resetSettingsStreaming, drawPlayControls, startStreaming) = "streaming.idle"_s,
            "streaming.preflight"_s + longPress = "menu"_s,
            "streaming.idle"_s + longPress / emergencyStop = "menu"_s,
            "streaming.idle"_s + event<ReturnToMenu> / emergencyStop = "menu"_s,
            "streaming.idle"_s + buttonPress / incrementControlStreaming = "streaming.idle"_s,

            "update"_s [isOnline] / (drawUpdate, startUpdate) = "update.checking"_s,
            "update"_s = "wifi"_s,
            "update.checking"_s + updateUnavailable / drawNoUpdate = "update.idle"_s,
            "update.idle"_s + buttonPress = "menu"_s,

            "wifi"_s / drawWiFi = "wifi.idle"_s,
            "wifi.idle"_s + done / stopWifiPortal = "menu"_s,
            "wifi.idle"_s + buttonPress / stopWifiPortal = "menu"_s,
            "wifi.idle"_s + longPress / resetWiFi = "restart"_s,

            "help"_s / drawHelp = "help.idle"_s,
            "help.idle"_s + buttonPress = "menu"_s,

            "error"_s / drawError = "error.idle"_s,
            "error.idle"_s + buttonPress / drawHelp = "error.help"_s,
            "error.help"_s + buttonPress / restart = X,

            "restart"_s / restart = X);

        // clang-format on
    }
};

#endif  // OSSM_STATE_MACHINE_H
