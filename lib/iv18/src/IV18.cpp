#include "IV18.h"
#include "SPI.h"

#define GET_BIT(v,n) ((v) & ((uint8_t)1 << (n)))    //获取v的第n位

IV18::IV18() {
    queryMap = {
        {"0", 0xFC},
        {"1", 0x60},
        {"2", 0xDA},
        {"3", 0xF2},
        {"4", 0x66},
        {"5", 0xB6},
        {"6", 0xBE},
        {"7", 0xE0},
        {"8", 0xFE},
        {"9", 0xF6},
        {" ", 0x00},
        {"A", 0xEE},
        {"B", 0x3E},
        {"C", 0x9C},
        {"D", 0x7A},
        {"E", 0x9E},
        {"F", 0x8E},
        {"G", 0xBC},
        {"H", 0x6E},
        {"I", 0x60},
        {"J", 0x70},
        {"K", 0x00},
        {"L", 0x1C},
        {"M", 0x00},    //Disabled
        {"N", 0xEC},
        {"O", 0xFC},
        {"P", 0xCE},
        {"Q", 0xFD},
        {"R", 0x0A},
        {"S", 0xB6},
        {"T", 0x1E},
        {"U", 0x7C},
        {"V", 0x38},    
        {"W", 0x1E},    //Disabled
        {"X", 0x00},    //Disabled
        {"Y", 0x76},   
        {"Z", 0x00},    //Disabled
        {"-", 0x02},
        {".", 0x01},
    };
    /*Initialize SPI*/
	SPI.begin();
	SPI.setFrequency(20000000);
	SPI.setBitOrder(LSBFIRST);
}

/**
 * Start a loop to refresh the IV18.
 */
void IV18::loopStart() {
	SPI.begin();
	int size = nowDisplaying.length();
    while(true) {
        if(size <= 8) {
            for (int counter = 0; counter < size; counter++) {
                uint32_t buffer = 0x00000001;
                buffer = buffer << (1 + counter + 8);  //左移1+counter,并为段选留8个bit
                buffer = buffer | queryMap[nowDisplaying.substring(counter, counter+1)];  //使用key值查找value
                if(GET_BIT(dotPlace, counter)) {
                    buffer = buffer | 0x01;
                }
                buffer = buffer << 15;
                SPI.transfer32(buffer);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
    }
}

/**
 * Set display content.
 * @param string What you want the IV18 Display.
 */
void IV18::setNowDisplaying(String str) {
    dotPlace = 0;
    str.toUpperCase();
	nowDisplaying = str;
} 

/**
 * Set display content.With dot(s).
 * @param string What you want the IV18 Display.
 * @param dot Where you want to add dot, HEX.
 */
void IV18::setNowDisplaying(String str, uint8_t dot) {
    dotPlace = dot;
    str.toUpperCase();
	nowDisplaying = str;
} 