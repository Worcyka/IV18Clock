#pragma once

#include "ArduinoJson.h"

class Weather {
public:
    Weather(uint32_t adcode);
    Weather();

    void city(uint32_t adcode);
    bool setCity(uint32_t adcode);
    bool cityValid();
    bool update();
    String weather();
    int8_t temperature();
    uint8_t humidity();
private:
    bool get(uint32_t adcode);

    String weatherc;
    int8_t temperaturec;
    uint8_t humidityc;
    uint32_t location;
};