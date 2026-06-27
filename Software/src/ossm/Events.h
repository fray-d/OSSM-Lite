#ifndef OSSM_SOFTWARE_EVENTS_H
#define OSSM_SOFTWARE_EVENTS_H

#include "boost/sml.hpp"
namespace sml = boost::sml;

/**
 * These are the events that the OSSM state machine can respond to.
 * They are used in OSSM.h and can be called from anywhere in the code that has
 * access to an OSSM state machine
 *
 * For Example:
 *  ossm->sm.process_event(ButtonPress{});
 *
 *
 * There's nothing special about these events, they are just structs.
 * They just happen to be defined inside of the OSSM State Machine class.
 */
struct ButtonPress {};

struct DoublePress {};
struct DoubleBoardPress {};

struct LongPress {};

struct Done {};

struct Error {};
struct EmergencyStop {};
struct Home {};

struct BleClick {};
struct ReturnToMenu {};

struct UpdateUnavailable {};
struct TryUpdate {};

// Definitions to make the table easier to read.
static auto bleClick = sml::event<BleClick>;
static auto buttonPress = sml::event<ButtonPress>;
static auto doublePress = sml::event<DoublePress>;
static auto doubleBoardPress = sml::event<DoubleBoardPress>;
static auto longPress = sml::event<LongPress>;
static auto done = sml::event<Done>;
static auto error = sml::event<Error>;
static auto home = sml::event<Home>;
static auto returnToMenu = sml::event<ReturnToMenu>;
static auto updateUnavailable = sml::event<UpdateUnavailable>;
static auto tryUpdate = sml::event<TryUpdate>;
#endif  // OSSM_SOFTWARE_EVENTS_H
