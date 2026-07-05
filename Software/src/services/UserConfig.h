#ifndef OSSM_SOFTWARE_USERCONFIG_H
#define OSSM_SOFTWARE_USERCONFIG_H

#include "Arduino.h"

namespace UserConfig {
    constexpr float sensorlessCurrentLimit = 1.5f;

    enum HomingType {None, Default, SingleSided, DoubleTap};

    float getSpeedCurve();

    std::string getDeviceName();
    void setDeviceName(String value);

    bool getDirection();
    void setDirection(bool value);
    void reverseDirection();

    HomingType getHomingType();
    void setHomingType(HomingType value);

    float getRailLength();
    void setRailLength(float value);

    float getMaxAcceleration();
    void setMaxAcceleration(float value);

    bool getReHome();
    void setReHome(bool value);

    float getMotorRPM();
    void setMotorRPM(float value);

    float getMotorStepsPR();
    void setMotorStepsPR(float value);

    float getPulleyTeeth();
    void setPulleyTeeth(float value);

    float getBeltPitch();
    void setBeltPitch(float value);
    
    float getStepsPerMM(float value = 1);
    void setStepsPerMM();

    float getMaxSpeedMMS();
    void setMaxSpeedMMS();

    float getMaxRailLength();
    constexpr float minStrokeLengthMm = 50.0;
}
#endif  // OSSM_SOFTWARE_USERCONFIG_H
