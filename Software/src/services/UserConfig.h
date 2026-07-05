#ifndef OSSM_SOFTWARE_USERCONFIG_H
#define OSSM_SOFTWARE_USERCONFIG_H

#include "Arduino.h"

namespace UserConfig {
    enum HomingType {None, Default, SingleSided, DoubleTap};

    float getSpeedCurve();
    void setSpeedCurve(float value);

    float getSensorLimit();
    void setSensorLimit(float value);

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
    float getMinRailLength();
}
#endif  // OSSM_SOFTWARE_USERCONFIG_H
