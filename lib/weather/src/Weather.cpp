#include "Weather.h"
#include "WiFi.h"

/**
 * @param uint32_t adcode of your city.
 */
Weather::Weather(uint32_t adcode) : location(adcode) {
}

Weather::Weather() {
    location = 100000;
}

/**
 * reset location.
 */
void Weather::city(uint32_t adcode) {
    location = adcode;
}

/**
 * Set location.
 * @param uint32_t adcode of your city.
 * @return return false if adcode invalid.
 */
bool Weather::setCity(uint32_t adcode) {
    if(adcode >= 100000 && adcode <= 900000) {
        location = adcode;
        return true;
    }
    return false;
}

/**
 * @return return false if adcode invalid.
 */
bool Weather::cityValid() {
    if(location >= 100000 && location<= 900000) {
        return true;
    }
    return false;
}

/**
 * Get Weather Now.
 * Make sure MCU is connected to WiFi before use this function.
 * @return Return true if updated successfully.
 */
bool Weather::get(uint32_t adcode) {
    if(WiFi.status() != WL_CONNECTED) {
        return 0;
    }

    const char* host = "restapi.amap.com";
    String reqRes = String("/v3/weather/weatherInfo?key=")
                  + "416a2e13b2ff847b64c6b1b9ed83f590"          //私钥
                  + "&city=" + adcode                           //城市adcode
                  + "&extensions" + "base";

    WiFiClient client;
    String httpRequest = String("GET ") + reqRes + " HTTP/1.1\r\n" 
                       + "Host: " + host + "\r\n"
                       + "Connection: close\r\n\r\n";

    if (client.connect(host, 80)) {
        client.print(httpRequest);
        Serial.println("Sending request: ");
        Serial.println(httpRequest);  
        // 获取并显示服务器响应状态行 
        String status_response = client.readStringUntil('\n');
        Serial.print("status_response: ");
        Serial.println(status_response);
        if(client.find("\r\n\r\n")) {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, client);

            if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return 0;
            }           

            JsonObject weatherNow = doc["lives"][0];
            const char* weatherChar;
            temperaturec = weatherNow["temperature"];
            humidityc    = weatherNow["humidity"];
            weatherChar  = weatherNow["weather"];

            weatherc = weatherChar;
        }else {
            return 0;
        }
    }else {
        Serial.println("Connection Failed!");
        return 0;
    }
    client.stop();
    return 1;
}

/**
 * Get Weather Now.
 * Make sure MCU is connected to WiFi before use this function.
 * @return Return true if updated successfully.
 */
bool Weather::update() {
    if(this->cityValid()) {
        return this->get(location);
    }else {

    }
}

/**
 * Get weather.
 * @return Return weather in ENG(String)
 */
String Weather::weather() {
/*M,W,X,Z Disabled*/
    if(weatherc == "晴") {
        return "Sunny";
    }else if(weatherc == "晴") {
        return "PtCloudy";
    }else if(weatherc == "少云") {
        return "PtCloudy";
    }else if(weatherc == "晴间多云" || weatherc == "多云") {
        return "Cloudy";
    }else if(weatherc == "阴") {
        return "Overcast";
    }else if(weatherc == "有风") {
        return "UJindy";
    }else if(weatherc == "平静"){
        return "Quiet";
    }else if(weatherc == "微风" || weatherc == "和风" || weatherc == "清风"){
        return "Breezy";
    }

    return "Gale";
}

/**
 * Get temperature.
 * @return Return temperature (uint8_t)
 */
int8_t Weather::temperature() {
    return temperaturec;
}

/**
 * Get humidity.
 * @return Return humidity (uint8_t)
 */
uint8_t Weather::humidity() {
    return humidityc;
}