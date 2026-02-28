/**
 *   StrokeEngine
 *   A library to create a variety of stroking motions with a stepper or servo
 * motor on an ESP32. https://github.com/theelims/StrokeEngine
 *
 * Copyright (C) 2022 theelims <elims@gmx.net>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#pragma once

#include <Arduino.h>

#include "FastAccelStepper.h"
#include "pattern.h"

// Debug Levels
// #define DEBUG_TALKATIVE             // Show debug messages from the
// StrokeEngine on Serial #define DEBUG_STROKE                // Show debug
// messaged for each individual stroke on Serial
#define DEBUG_CLIPPING  // Show debug messages when motions violating the
                        // machine physics are commanded

/**************************************************************************/
/*!
  @brief  Struct defining the physical properties of the stroking machine.
*/
/**************************************************************************/
typedef struct {
    float physicalTravel;  /*> What is the maximum physical travel in mm */
    float keepoutBoundary; /*> Soft endstop preventing hard crashes in mm. Will
                            * be subtracted twice from physicalTravel. Should be
                            *  sufficiently to completley drive clear from
                            *  homing switch */
} machineGeometry;

/**************************************************************************/
/*!
  @brief  Struct defining the motor (stepper or servo with STEP/DIR
  interface) and the motion system translating the rotation into a
  linear motion.
*/
/**************************************************************************/
typedef struct {
    float maxSpeed;           /*> What is the maximum speed in mm/s */
    float maxAcceleration;    /*> Maximum acceleration in mm/s^2 */
    float stepsPerMillimeter; /*> Number of steps per millimeter */
    bool invertDirection;     /*> Set to true to invert the direction signal
                               *  The firmware expects the home switch to be located
                               * at the     end of an retraction move. That way the
                               * machine homes     itself away from the body. Home
                               * position is -KEEPOUTBOUNDARY */
    bool enableActiveLow;     /*> Polarity of the enable signal. True for active
                                 low. */
    int stepPin;              /*> Pin connected to the STEP input */
    int directionPin;         /*> Pin connected to the DIR input */
    int enablePin;            /*> Pin connected to the ENA input */
} motorProperties;

/**************************************************************************/
/*!
  @brief  Enum containing the states of the state machine
*/
/**************************************************************************/
typedef enum {
    UNDEFINED,   //!< No power to the servo. We don't know its position
    READY,       //!< Servo is energized and knows it position. Not running.
    PATTERN      //!< Stroke Engine is running and servo is moving according to
                 //!< defined pattern.
} ServoState;

// Verbose strings of states for debugging purposes
static String verboseState[] = {
    "[0] Servo disabled", "[1] Servo ready", "[2] Servo pattern running",
    "[3] Servo setup depth", "[4] Servo position streaming"};

/**************************************************************************/
/*!
  @brief  Stroke Engine provides a convenient package for stroking motions
  created by stepper or servo motors. It's internal states are handled by a
  finite state machine. A pattern generator allows to creat a variety of
  motion profiles. Under the hood FastAccelStepper is used for interfacing
  a stepper or servo motor vie a STEP/DIR interface.
*/
/**************************************************************************/
class StrokeEngine {
    FastAccelStepper *_servo;

  public:
    /**************************************************************************/
    /*!
      @brief  Initializes FastAccelStepper and configures all pins and outputs
      accordingly. StrokeEngine is in state UNDEFINED
    */
    /**************************************************************************/
    void begin(machineGeometry *physics, motorProperties *motor,
               FastAccelStepper *servo);

    /**************************************************************************/
    /*!
      @brief  Set the speed of a stroke. Speed is given in Strokes per Minute
      and internally calculated to the time a full stroke needs to complete.
      Settings tale effect with next stroke, or after calling
      applyNewSettingsNow().
      @param speed Strokes per Minute. Is constrained from 0.5 to 6000
      @param applyNow Set to true if changes should take effect immediately
    */
    /**************************************************************************/
    void setSpeed(float speed, bool applyNow);

    /**************************************************************************/
    /*!
      @brief  Set the depth of a stroke. Settings tale effect with next stroke,
      or after calling applyNewSettingsNow().
      @param depth Depth in [mm]. Is constrained from 0 to TRAVEL
      @param applyNow Set to true if changes should take effect immediately
    */
    /**************************************************************************/
    void setDepth(float depth, bool applyNow);

    /**************************************************************************/
    /*!
      @brief  Set the stroke length of a stroke. Settings take effect with next
      stroke, or after calling applyNewSettingsNow().
      @param stroke Stroke length in [mm]. Is constrained from 0 to TRAVEL
      @param applyNow Set to true if changes should take effect immediately
    */
    /**************************************************************************/
    void setStroke(float stroke, bool applyNow);

    /**************************************************************************/
    /*!
      @brief  Set the sensation of a pattern. Sensation is an additional
      parameter a pattern may use to alter its behaviour. Settings takes
      effect with next stroke, or after calling applyNewSettingsNow().
      @param sensation  Sensation in [a.u.]. Is constrained from -100 to 100
                    with 0 beeing assumed as neutral.
      @param applyNow Set to true if changes should take effect immediately
    */
    /**************************************************************************/
    void setSensation(float sensation, bool applyNow);

    /**************************************************************************/
    /*!
      @brief  Choose a pattern for the StrokeEngine. Settings take effect with
      next stroke, or after calling applyNewSettingsNow().
      @param patternIndex  Index of a pattern
      @param applyNow Set to true if changes should take effect immediately
      @return TRUE on success, FALSE, if patternIndex is invalid. Previous
                    pattern will be retained.
    */
    /**************************************************************************/
    bool setPattern(Pattern *nextPattern, bool applyNow);

    /**************************************************************************/
    /*!
      @brief  Creates a FreeRTOS task to run a stroking pattern. Only valid in
      state READY. Pattern is initialized with the values from the set
      functions. If the task is running, state is PATTERN.
      @return TRUE when task was created and motion starts, FALSE on failure.
    */
    /**************************************************************************/
    bool startPattern();

    /**************************************************************************/
    /*!
      @brief  Stops the motion with MAX_ACCEL and deletes the stroking task. Is
      in state READY afterwards.
    */
    /**************************************************************************/
    void stopMotion();

    /**************************************************************************/
    /*!
      @brief  If no homing switch is present homing can be done manually. Push
      the endeffector all the way in and call thisIsHome(). This enables the
      the servo and sets the position to -KEEPOUT_BOUNDARY
      @param speed  Speed in mm/s used for finding the homing switch.
                    Defaults to 5.0 mm/s
    */
    /**************************************************************************/
    void thisIsHome(float speed = 5.0);

    /**************************************************************************/
    /*!
      @brief  Retrieves the current servo state from the internal state machine.
      @return Current state of the state machine
    */
    /**************************************************************************/
    ServoState getState();

    /**************************************************************************/
    /*!
      @brief  Register a callback function that will update telemetry
      information about StrokeEngine. The provided function will be called
      whenever a motion is executed by a manual command or by a pattern. The
      returned values are the target position of this move, its top speed and
      wether clipping occurred.
      @param callbackTelemetry Function must be of type:
      void callbackTelemetry(float position, float speed, bool clipping)
    */
    /**************************************************************************/
    void registerTelemetryCallback(void (*callbackTelemetry)(float, float,
                                                             bool));

  protected:
    ServoState _state = UNDEFINED;
    motorProperties *_motor;
    machineGeometry *_physics;
    float _travel;
    int _minStep;
    int _maxStep;
    int _maxStepPerSecond;
    int _maxStepAcceleration;
    Pattern *pattern = new SimpleStroke("Simple Stroke");
    bool _isHomed = false;
    int _index = 0;
    int _depth;
    int _previousDepth;
    int _stroke;
    int _previousStroke;
    float _timeOfStroke;
    float _sensation;
    bool _applyUpdate = false;
    static void _strokingImpl(void *_this) {
        static_cast<StrokeEngine *>(_this)->_stroking();
    }
    void _stroking();
    TaskHandle_t _taskStrokingHandle = NULL;
    TaskHandle_t _taskHomingHandle = NULL;
    TaskHandle_t _taskStreamingHandle = NULL;
    SemaphoreHandle_t _patternMutex = xSemaphoreCreateMutex();
    void _applyMotionProfile(motionParameter *motion);
    void (*_callBackHomeing)(bool) = NULL;
    void (*_callbackTelemetry)(float, float, bool) = NULL;
    int _homeingSpeed;
    int _homeingPin;
    int _homeingToBack;
    bool _homeingActiveLow; /*> Polarity of the homing signal*/
    bool _fancyAdjustment;
};
