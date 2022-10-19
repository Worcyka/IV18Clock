#ifndef IV18DRIVER_H
#define IV18DRIVER_H

#include <Arduino.h>
#include <map>

class IV18 {
public:
    IV18();

    void loopStart();
    void setNowDisplaying(String str);
    void setNowDisplaying(String str, uint8_t dot);
private:
    String nowDisplaying;
    std::map<String, uint8_t> queryMap;
    uint8_t dotPlace;
};

#endif