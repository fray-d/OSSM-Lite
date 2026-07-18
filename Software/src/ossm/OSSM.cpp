#include "OSSM.h"

#include "command/commands.hpp"
#include "ossm/state/settings.h"
#include "ossm/state/state.h"
#include "services/communication/queue.h"
#include "services/encoder.h"

namespace sml = boost::sml;
using namespace sml;

// Global OSSM pointer (kept for backward compatibility during migration)
OSSM *ossm = nullptr;

void OSSM::ble_click(String commandString) {
    CommandValue command = commandFromString(commandString);
    ESP_LOGD("OSSM", "COMMAND: %d", command.command);

    String currentState;
    if (stateMachine != nullptr) {
        stateMachine->visit_current_states(
            [&currentState](auto state) { currentState = state.c_str(); });
    }

    switch (command.command) {
        case Commands::goToStrokeEngine:
            menuState.currentOption = Menu::StrokeEngine;
            if (stateMachine != nullptr) {
                stateMachine->process_event(ButtonPress{});
            }
            break;
        case Commands::goToStreaming:
            menuState.currentOption = Menu::Streaming;
            if (stateMachine != nullptr) {
                stateMachine->process_event(ButtonPress{});
            }
            break;
        case Commands::goToMenu:
            if (stateMachine != nullptr) {
                stateMachine->process_event(ReturnToMenu{});
            }
            break;
        case Commands::setSpeed:
            // BLE devices can be trusted to send true value
            // and can bypass potentiomer smoothing logic
            bleState.lastSpeedCommandWasFromBLE = true;
            // Use speed knob config to determine how to handle BLE speed
            // command
            settings.speedBLE = command.value;
            break;
        case Commands::setDepth:
            command.value = constrain(command.value, 1.0f, 100.0f);
            settings.minPosition = command.value - settings.stroke;
            settings.minPosition = constrain(settings.minPosition, 0.0f, 100.0f);
            [[fallthrough]]; 
        case Commands::setMaxPosition:
            command.value = constrain(command.value, 1.0f, 100.0f);
            settings.playControl = ui::PlayControls::MAX_POSITION;
            encoder.setEncoderValue(command.value);
            settings.maxPosition = command.value;
            break;
        case Commands::setMinPosition:
            settings.minPosition = constrain(command.value, 0.0f, 99.0f);
            settings.playControl = ui::PlayControls::MIN_POSITION;
            encoder.setEncoderValue(settings.minPosition);
            settings.stroke = settings.maxPosition - settings.minPosition;
            break;
        case Commands::setStroke:
            // Legacy alias: set:stroke:X → minPosition = maxPosition - value
            settings.stroke = constrain(command.value, 0.0f, 100.0f);
            settings.minPosition = settings.maxPosition - settings.stroke;
            settings.minPosition = constrain(settings.minPosition, 0.0f, 100.0f);
            settings.playControl = ui::PlayControls::MIN_POSITION;
            encoder.setEncoderValue(settings.minPosition);
            break;
        case Commands::setSensation:
            settings.playControl = ui::PlayControls::SENSATION;
            encoder.setEncoderValue(command.value);
            settings.sensation = command.value;
            break;
        case Commands::setBuffer:
            settings.playControl = ui::PlayControls::BUFFER;
            encoder.setEncoderValue(command.value);
            settings.buffer = command.value;
            break;
        case Commands::setPattern:
            settings.pattern = static_cast<StrokePatterns>((int)command.value % (int)StrokePatterns::Count);
            break;
        case Commands::streamPosition:
            // Position (0-100)
            targetQueue.push({
                static_cast<float>(command.value),
                static_cast<uint16_t>(command.time),
                std::chrono::steady_clock::now(), 0});
            break;
        case Commands::setWifi:
        case Commands::ignore:
            break;
    }
}

String OSSM::getStateFingerprint() {
    String currentState;
    if (stateMachine != nullptr) {
        stateMachine->visit_current_states(
            [&currentState](auto state) { currentState = state.c_str(); });
    }

    String output = currentState + ":";
    output += String((int)settings.speed) + ":";
    output += String((int)settings.minPosition) + ":";
    output += String((int)settings.maxPosition) + ":";
    output += String((int)settings.sensation) + ":";
    output += String(static_cast<int>(settings.pattern)) + ":";
    return output;
}

String OSSM::getCurrentState() {
    String currentState;
    if (stateMachine != nullptr) {
        stateMachine->visit_current_states(
            [&currentState](auto state) { currentState = state.c_str(); });
    }

    float stroke = 0.0;
    if (settings.maxPosition + settings.minPosition > 0.0) {
        stroke = (settings.maxPosition - settings.minPosition) / settings.maxPosition * 100.0;
    }

    return "{\"timestamp\":" + String((unsigned long)millis()) +
           ",\"state\":\"" + currentState +
           "\",\"speed\":" + String(settings.speed) +
           ",\"minPosition\":" + String(settings.minPosition) +
           ",\"maxPosition\":" + String(settings.maxPosition) +
           ",\"depth\":" + String(settings.maxPosition) +
           ",\"stroke\":" + String(stroke) +
           ",\"sensation\":" + String(settings.sensation) +
           ",\"buffer\":" + String(settings.buffer) + 
           ",\"pattern\":" + String(static_cast<int>(settings.pattern)) + 
           "}";
}
