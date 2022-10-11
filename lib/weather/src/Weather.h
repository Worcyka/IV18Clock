#pragma once

#include "ArduinoJson.h"

class Weather {
public:
    Weather(uint32_t adcode);
    Weather();

    void city(uint32_t adcode);
    bool setCity(uint32_t adcode);
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