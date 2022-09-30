#ifndef IV18DRIVER_H
#define IV18DRIVER_H

#include <Arduino.h>
#include <map>

class IV18 {
public:
    IV18();

    void loopStart();
    void setNowDisplaying(String str);
private:
    String nowDisplaying;
    std::map<String, uint8_t> queryMap;
};

#endif