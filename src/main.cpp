#include <Arduino.h>
#include "ThreeWire.h"
#include "RtcDS1302.h"
#include "RtcDateTime.h"
#include "freertos/task.h"
#include "DispManager.h"
#include "esp_timer.h"
#include "NTPClient.h"
#include "WiFiUdp.h"
#include "IV18.h"
#include "WiFi.h"

IV18 iv18;
ThreeWire i2cWire(19, 21, 15);
RtcDS1302<ThreeWire> rtc(i2cWire);
WiFiUDP udp;
NTPClient ntp(udp);
RtcDateTime timeNow;

hw_timer_t *timer = NULL;

void displayLoop(void *param) {
	iv18.loopStart();
}

void getNTPTime(void *param) {
	while(true) {
		WiFi.begin("2906 Wireless Network", "25617192");
		while(WiFi.status() != WL_CONNECTED) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}	//连接WiFi
		Serial.println("WiFi Connected!");
		ntp.begin();
		ntp.update();	//获取更新的时间
		RtcDateTime rtcNtp(ntp.getEpochTime() - 946656000);  //时间戳转换成自2000以后的时间
		rtc.SetDateTime(rtcNtp);
		Serial.println("Time Updated!");
		WiFi.disconnect();
		WiFi.getSleep();
		vTaskDelay(pdMS_TO_TICKS(86400000));	//一天后更新
	}
}

String formatTime(RtcDateTime timeNow) {
	String time;
	if(timeNow.Hour() < 10) {
		time = time + "0" + timeNow.Hour();
	}else {
		time = time + timeNow.Hour();
	}
	if(timeNow.Second() % 2) {
		time = time + ".";
	}else {
		time = time + " ";
	}
	if(timeNow.Minute() < 10) {
		time = time + "0" + timeNow.Minute();
	}else {
		time = time + timeNow.Minute();
	}
	if(timeNow.Second() % 2) {
		time = time + ".";
	}else {
		time = time + " ";
	}
	if(timeNow.Second() < 10) {
		time = time + "0" + timeNow.Second();
	}else {
		time = time + timeNow.Second();
	}
	return time;
}

void IRAM_ATTR getRTCTime() {
	timeNow = rtc.GetDateTime();
	Serial.println("Time Updated By RTC");
}

void setup() {
	Serial.begin(9600);
	iv18.setNowDisplaying("        "); 
	timeNow = rtc.GetDateTime();

	timer = timerBegin(0, 80, true);                    //定时器初始化--80Mhz分频80，则时间单位为1Mhz即1us即10-6s
  	timerAttachInterrupt(timer, &getRTCTime, true); 	//中断绑定定时器
  	timerAlarmWrite(timer, 1800000000, true);           //每30分钟用RTC时间更新一次
  	timerAlarmEnable(timer);    

	xTaskCreate(displayLoop, "dispLoop", 1024 * 1, NULL, 32, NULL);
	xTaskCreate(getNTPTime , "Regulate", 1024 * 4, NULL, 31, NULL);
}

void loop() {      
	iv18.setNowDisplaying(formatTime(timeNow));
	vTaskDelay(pdMS_TO_TICKS(1000));
	timeNow += 1; //time续一秒
}