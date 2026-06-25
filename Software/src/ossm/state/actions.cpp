#include "actions.h"

#include "ossm/advanced_penetration/advanced_penetration.h"
#include "ossm/homing/homing.h"
#include "ossm/menu/menu.h"
#include "ossm/pages/error.h"
#include "ossm/pages/hello.h"
#include "ossm/pages/help.h"
#include "ossm/pages/preflight.h"
#include "ossm/pages/update.h"
#include "ossm/pages/wifi.h"
#include "ossm/pattern_controls/pattern_controls.h"
#include "ossm/play_controls/play_controls.h"
#include "ossm/state/settings.h"
#include "ossm/streaming/streaming.h"
#include "ossm/stroke_engine/stroke_engine.h"
#include "services/encoder.h"
#include "services/UserConfig.h"
#include "services/stepper.h"
#include "services/wm.h"
#include "ui.h"

void ossmDrawHello() { pages::drawHello(); }

void ossmDrawMenu() { menu::drawMenu(); }

void ossmStartHoming() {
    homing::clearHoming();
    homing::startHoming();
}

void ossmDrawPlayControls() {
    play_controls::drawPlayControls();
}

void ossmStartStreaming() {
    streaming::startStreaming();
}

void ossmDrawPatternControls() {
    pattern_controls::drawPatternControls();
}

void ossmDrawPreflight() { pages::drawPreflight(); }

void ossmResetSettingsStrokeEngine() {
    settings.speed = 0;
    settings.speedBLE = 0;
    settings.minPosition = 0;
    settings.maxPosition = 20;
    settings.sensation = 50;
    settings.playControl = ui::PlayControls::MAX_POSITION;

    // Prepare the encoder
    encoder.setBoundaries(0, 100, false);
    encoder.setAcceleration(10);
    encoder.setEncoderValue(settings.maxPosition);
}
void ossmResetSettingsStreaming() {
    settings.speed = 0;
    settings.minPosition = 50;
    settings.maxPosition = 50;
    settings.sensation = 50;
    settings.buffer = 100;

    // Prepare the encoder
    encoder.setBoundaries(0, 100, false);
    encoder.setAcceleration(10);
    encoder.setEncoderValue(settings.maxPosition);
}

void ossmAdvancedClick() {
    advanced_penetration::advancedClick();
}

void ossmIncrementControlStrokeEngine() {
    settings.playControl = static_cast<ui::PlayControls>((int(settings.playControl) + 1) % 3);
    switch (settings.playControl) {
        case ui::PlayControls::MIN_POSITION:
            encoder.setEncoderValue(settings.minPosition);
            break;
        case ui::PlayControls::MAX_POSITION:
            encoder.setEncoderValue(settings.maxPosition);
            break;
        case ui::PlayControls::SENSATION:
            encoder.setEncoderValue(settings.sensation);
            break;
        default:
            break;
    }
}

void ossmIncrementControlStreaming() {
    settings.playControl = static_cast<ui::PlayControls>((int(settings.playControl) + 1) % 4);
    switch (settings.playControl) {
        case ui::PlayControls::MIN_POSITION:
            encoder.setEncoderValue(settings.minPosition);
            break;
        case ui::PlayControls::MAX_POSITION:
            encoder.setEncoderValue(settings.maxPosition);
            break;
        case ui::PlayControls::SENSATION:
            encoder.setEncoderValue(settings.sensation);
            break;
        case ui::PlayControls::BUFFER:
            encoder.setEncoderValue(settings.buffer);
            break;
    }
};

void ossmStartAdvancedPenetration() {
    advanced_penetration::startAdvancedPenetration();
}

void ossmStartStrokeEngine() {
    stroke_engine::startStrokeEngine();
}
void ossmEmergencyStop() {
    stepper->forceStop();
    stepper->disableOutputs();
    ossmSetNotHomed();
}

void ossmDrawHelp() { pages::drawHelp(); }

void ossmDrawWiFi() { pages::drawWiFi(); }

void ossmDrawUpdate() { pages::drawUpdate(); }

void ossmDrawNoUpdate() { pages::drawNoUpdate(); }

void ossmDrawUpdating() { pages::drawUpdating(); }

void ossmDrawError() { pages::drawError(); }

void ossmSetFirstHomed() {
    calibration.isFirstHomed = false;
}

void ossmSetHomed() {
    calibration.isHomed = true;
    ossmSetFirstHomed();
}

void ossmSetNotHomed() {
    if(UserConfig::getReHome()) {
        calibration.isHomed = false;
    }
}

void ossmResetWiFi() { wm.resetSettings(); }

void ossmRestart() { ESP.restart(); }
