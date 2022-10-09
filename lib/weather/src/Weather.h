#pragma once

#include "ArduinoJson.h"

class Weather {
public:
    Weather(uint32_t abcode);

    void city(uint32_t abcode);
    bool update();
    String weather();
    uint8_t temperature();
    uint8_t humidity();
private:
    String weatherc;
    uint8_t temperaturec;
    uint8_t humidityc;
    uint32_t location;
};