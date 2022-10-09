#pragma once

#include "ArduinoJson.h"

class Weather {
public:
    Weather(uint32_t abcode);

    void city(uint32_t abcode);
    bool update();
    String weather();
    int8_t temperature();
    uint8_t humidity();
private:
    String weatherc;
    int8_t temperaturec;
    uint8_t humidityc;
    uint32_t location;
};