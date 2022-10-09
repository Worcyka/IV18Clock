#pragma once

#include "ArduinoJson.h"

class Weather {
public:
    Weather(int loc);

    void city();
    bool update();
    String weather();
    uint8_t temperature();
    uint8_t humidity();
private:
    String weatherc;
    uint8_t temperaturec;
    uint8_t humidityc;
    uint8_t location;
};