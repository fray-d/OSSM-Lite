#ifndef OSSM_ADVANCED_PENETRATION_STRINGS_H
#define OSSM_ADVANCED_PENETRATION_STRINGS_H

#include "Progmem.h"

namespace advanced_penetration {

    static const char d1[] PROGMEM = "+D";
    static const char d2[] PROGMEM = "-D";
    static const char s1[] PROGMEM = "+S";
    static const char s2[] PROGMEM = "-S";
    static const char a1[] PROGMEM = "+A";
    static const char a2[] PROGMEM = "-A";
    static const char ma[] PROGMEM = "MA";
    static const char m1[] PROGMEM = "M1";
    static const char m2[] PROGMEM = "M2";
    static const char m3[] PROGMEM = "M3";
    static const char m4[] PROGMEM = "M4";
    static const char mo[] PROGMEM = "MO";
    static const char sp[] PROGMEM = "SP";

    static const char p0[] PROGMEM = "Simple";
    static const char c0[] PROGMEM = "0:0:100,1:0:100,2:100,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p1[] PROGMEM = "Teasing";
    static const char c1[] PROGMEM = "0:0:100,1:0:100,2:50,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p2[] PROGMEM = "Pounding";
    static const char c2[] PROGMEM = "0:0:100,1:0:100,2:100,2:0:100,3:50,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p3[] PROGMEM = "Robo";
    static const char c3[] PROGMEM = "0:0:100,1:0:100,2:100,2:0:100,3:100,3:0:100,4:100,4:0:100,5:100,5:0:100,";
    static const char p4[] PROGMEM = "Half'n'half";
    static const char c4[] PROGMEM = "0:0:50,1:0:100,2:100,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p5[] PROGMEM = "Deeper";
    static const char c5[] PROGMEM = "0:0:15,0:1:1,0:2:0,0:3:10,0:4:0,0:5:0,1:0:100,2:100,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p6[] PROGMEM = "Insist";
    static const char c6[] PROGMEM = "0:0:100,1:0:15,1:1:10,1:2:0,1:3:0,1:4:0,1:5:0,2:100,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p7[] PROGMEM = "Jackhammer";
    static const char c7[] PROGMEM = "0:0:15,0:1:1,0:2:0,0:3:10,0:4:0,0:5:0,1:0:15,1:1:10,1:2:0,1:3:0,1:4:0,1:5:0,2:100,2:0:100,3:100,3:0:50,3:1:1,3:2:9,3:3:1,3:4:0,3:5:0,4:100,4:0:100,5:0,5:0:100,";
    static const char p8[] PROGMEM = "Progressive";
    static const char c8[] PROGMEM = "0:0:15,0:1:10,0:2:1,0:3:10,0:4:0,0:5:0,1:0:15,1:1:10,1:2:1,1:3:10,1:4:0,1:5:11,2:100,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";
    static const char p9[] PROGMEM = "Mid";
    static const char c9[] PROGMEM = "0:0:15,0:1:10,0:2:1,0:3:10,0:4:0,0:5:0,1:0:15,1:1:10,1:2:1,1:3:10,1:4:0,1:5:0,2:100,2:0:100,3:100,3:0:100,4:40,4:0:100,5:40,5:0:100,";

    static const char* const presets[10] = {
            p0, p1, p2, p3, p4, 
            p5, p6, p7, p8, p9,
    };

    static const char* const presetCommands[10] = {
            c0, c1, c2, c3, c4, 
            c5, c6, c7, c8, c9,
    };

}

#endif