#ifndef OSSM_SOFTWARE_USERCONFIG_H
#define OSSM_SOFTWARE_USERCONFIG_H

#include "Arduino.h"

namespace UserConfig {
    float getSpeedCurve();

    std::string getDeviceName();
    void setDeviceName(String value);

    bool getDirection();
    void setDirection(bool value);

    enum HomingType {None, Default, SingleSided, DoubleTap};

    HomingType getHomingType();
    void setHomingType(HomingType value);

    float getRailLength();
    void setRailLength(float value);

    bool getReHome();
    void setReHome(bool value);

}
#endif  // OSSM_SOFTWARE_USERCONFIG_H
