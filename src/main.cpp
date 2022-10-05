#include <Arduino.h>
#include "ThreeWire.h"
#include "RtcDS1302.h"
#include "RtcDateTime.h"
#include "ArduinoJson.h"
#include "freertos/task.h"
#include "WiFiClientSecure.h"
#include "DispManager.h"
#include "esp_timer.h"
#include "NTPClient.h"
#include "WiFiUdp.h"
#include "SPIFFS.h"
#include "IV18.h"
#include "WiFi.h"

#define MAXPAGE 2		//最多允许多少页，请务必正确配置！
#define TRIGVALUE 30	//触摸感应开关的触发阈值

static String fileName = "/config/wificonfig.json";

IV18 iv18;
ThreeWire i2cWire(19, 21, 15);
RtcDS1302<ThreeWire> rtc(i2cWire);
WiFiUDP udp;
NTPClient ntp(udp);
RtcDateTime timeNow;

hw_timer_t *updtByRTCTimer = NULL;
hw_timer_t *second = NULL;

bool trigged = false;
int dispPage = 0;

void displayLoop(void *param) {
	iv18.loopStart();
}

void getNTPTime(void *param) {
	while(true) {
		WiFi.begin("2906 Wireless Network", "25617192");
		while(WiFi.status() != WL_CONNECTED) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}													 //连接WiFi
		Serial.println("WiFi Connected!");
		ntp.begin();
		ntp.update();	                                     //获取更新的时间
		RtcDateTime rtcNtp(ntp.getEpochTime() - 946656000);  //时间戳转换成自2000以后的时间
		rtc.SetDateTime(rtcNtp);
		timeNow = rtc.GetDateTime();
		Serial.println("Time Updated!");
		WiFi.disconnect();						             //断开WiFi
		WiFi.getSleep();						             //WiFi睡眠
		for(int i = 0; i < 1440; i++) {
			vTaskDelay(pdMS_TO_TICKS(60000));
		}
	}
}

void resetter(void *param) {
	while(1) {
		if(touchRead(T0) > TRIGVALUE) {
			vTaskDelay(pdMS_TO_TICKS(100));
			trigged = false;
			vTaskDelete(NULL);
		}else {
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}

String formatTime(RtcDateTime timeNow) {
/**************Just Format Time**************/
	String time;
	if(timeNow.Hour() < 10) {
		if(timeNow.Hour() == 0) {
			time = "12";
		}else {
			time = time + "0" + timeNow.Hour();
		}
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

void dispManage() {
	switch(dispPage) {
		case 0:
			iv18.setNowDisplaying(formatTime(timeNow));			//显示当前时间
			break;
		case 1:
			if(WiFi.status() != WL_CONNECTED) {					//查询WiFi状态
				iv18.setNowDisplaying("NET  OFF");
			}else {
				iv18.setNowDisplaying("NET   ON");
			}
	}
}

void IRAM_ATTR changePage() {
	if(trigged == false) {
		xTaskCreate(resetter, "Resetter", 512, NULL, 31, NULL); //Resetter
		if(dispPage < MAXPAGE - 1) {
			dispPage++; 
			dispManage();
		}else {
			dispPage = 0;
			dispManage();
		}
	}
	trigged = true;
}

void IRAM_ATTR ticktock() {
	dispManage();
	timeNow += 1; //time续一秒
}

void IRAM_ATTR getRTCTime() {
	timeNow = rtc.GetDateTime();
	Serial.println("Time Updated By RTC");
}

void setup() {
	Serial.begin(9600);
	iv18.setNowDisplaying("        "); 
	timeNow = rtc.GetDateTime();								     //初始化时间

	updtByRTCTimer = timerBegin(0, 80, true);                        //定时器初始化--80Mhz分频80，则时间单位为1Mhz即1us即10-6s
  	timerAttachInterrupt(updtByRTCTimer, &getRTCTime, true); 	     //中断绑定定时器
  	timerAlarmWrite(updtByRTCTimer, 900000000, true);                //每15分钟用RTC时间更新一次
	timerAlarmEnable(updtByRTCTimer);							     //使能

	second = timerBegin(1, 80, true);							     //定时器初始化--80Mhz分频80，则时间单位为1Mhz即1us即10-6
	timerAttachInterrupt(second, &ticktock, true);				     //中断绑定定时器
	timerAlarmWrite(second, 1000000, true);						     //每1秒钟刷新显示时间
	timerAlarmEnable(second);							 		     //使能

	touchAttachInterrupt(T0, changePage, TRIGVALUE);				 //触摸按键绑定切换菜单

  	File dataFileRead = SPIFFS.open(fileName, "r"); 				 //建立File对象用于从SPIFFS中读取文件
	String wifiJsonRead;
  	for(int i=0; i<dataFileRead.size(); i++){
		wifiJsonRead = wifiJsonRead + (char)dataFileRead.read();     //读取文件内容
  	}
	Serial.println(wifiJsonRead);
  	dataFileRead.close();    										 //完成文件读取后关闭文件

	xTaskCreate(displayLoop, "dispLoop", 1024 * 1, NULL, 32, NULL);	 //显示进程
	xTaskCreate(getNTPTime , "Regulate", 1024 * 4, NULL, 31, NULL);	 //在线同步时间进程
}

void loop() {                       
	vTaskDelay(pdMS_TO_TICKS(1000000000));
}