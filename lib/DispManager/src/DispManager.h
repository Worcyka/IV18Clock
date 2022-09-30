#ifndef DISPMANAGER_H
#define DISPMANAGER_H

#include <Arduino.h>
#include "RtcDS1302.h"
#include "IV18.h"

class DispManager {
public:
    DispManager();
    DispManager(IV18* iv18T);

    void bind(IV18* iv18T);
    void setTime(RtcDateTime time);
    void manageLoop();
    void change();
private:
    IV18* iv18;
    RtcDateTime timeNow;
};

#endif