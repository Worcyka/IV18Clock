#include <Arduino.h>
#include "ThreeWire.h"
#include "RtcDS1302.h"
#include "RtcDateTime.h"
#include "ArduinoJson.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "WiFiClientSecure.h"
#include "WiFiMulti.h"
#include "esp_timer.h"
#include "NTPClient.h"
#include "Blinker.h"
#include "WiFiUdp.h"
#include "Weather.h"
#include "memory.h"
#include "SPIFFS.h"
#include "IV18.h"

#define MAXPAGE 6		//!最多允许多少页，请务必正确配置!
#define TRIGVALUE 20	//触摸感应开关的触发阈值
#define BLINKER_BLE		//使用BLINKER的蓝牙模式
#define BLINKER_PRINT Serial
#define DEFAULTMODE 0
#define SSID_SETTING 1
#define PASSWORD_SETTING 2
#define CHOOSE_WIFI 3

static String fileName = "/config/wificonfig.json";

BlinkerText text("text");

IV18 iv18;
ThreeWire i2cWire(19, 21, 15);
RtcDS1302<ThreeWire> rtc(i2cWire);
Weather weather(442000);
WiFiUDP udp;
NTPClient ntp(udp);
RtcDateTime timeNow;
WiFiMulti wifi;

hw_timer_t *updtByRTCTimer = NULL;
hw_timer_t *second = NULL;

SemaphoreHandle_t wifiSemaph = xSemaphoreCreateBinary();	//用于控制WiFI的信号量

bool trigged = false;
int dispPage = 0;
int mode = 0;

void displayLoop(void *param) {
	iv18.loopStart();
}

void getWeather(void *param) {
	while(true) {
		while(xSemaphoreTake(wifiSemaph, portMAX_DELAY) != pdFALSE &&
			 wifi.run() != WL_CONNECTED) {
			xSemaphoreGive(wifiSemaph);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		if(weather.update()) {
			Serial.println(weather.temperature());
			Serial.println(weather.humidity());
			Serial.println(weather.weather());
		}else {
			Serial.println("Failed!");
		}
		WiFi.disconnect();				            		 //断开WiFi
		WiFi.getSleep();						             //WiFi睡眠
		xSemaphoreGive(wifiSemaph);							 //释放信号量
		for(int i = 0; i < 60; ++i) {						 //小睡60分钟
			vTaskDelay(pdMS_TO_TICKS(60000));  
		}	
	}
}

void getNTPTime(void *param) {
	while(true) {
		while(xSemaphoreTake(wifiSemaph, portMAX_DELAY) != pdFALSE &&
			 wifi.run() != WL_CONNECTED) {
			xSemaphoreGive(wifiSemaph);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}													 //连接WiFi
		Serial.println("WiFi Connected!");
		ntp.begin();
		ntp.update();	                                     //获取更新的时间
		RtcDateTime rtcNtp(ntp.getEpochTime() - 946656000);  //时间戳转换成自2000以后的时间
		rtc.SetDateTime(rtcNtp);
		timeNow = rtc.GetDateTime();
		Serial.println("Time Updated!");
		WiFi.disconnect();				            		 //断开WiFi
		WiFi.getSleep();						             //WiFi睡眠
		xSemaphoreGive(wifiSemaph);							 //释放信号量	
		for(int i = 0; i < 1440; ++i) {						 //休眠一整天
			vTaskDelay(pdMS_TO_TICKS(60000));  
		}
	}
}

void resetter(void *param) {
	while(1) {
		if(touchRead(T0) > TRIGVALUE) {
			vTaskDelay(pdMS_TO_TICKS(300));
			trigged = false;
			vTaskDelete(NULL);
		}else {
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}

/************检测指令并依此配置WiFi*************/
void dataRead(const String &data) {
	static std::unique_ptr<String> pSSID;
	if(data == "quit" && mode != DEFAULTMODE) {
		text.print("Please Enter Directive");
		mode = DEFAULTMODE;
	}
	if(data == "clear config" && mode == DEFAULTMODE) {
		SPIFFS.format();
		text.print("All Config Cleared!", "");
		return;
	}
	if(data == "show config" && mode == DEFAULTMODE) {
		if(!SPIFFS.begin()){											 //启用SPIFFS
			text.print("SPIFFS Failed to Start!");
		}
  		File dataFileRead = SPIFFS.open(fileName, "r"); 				 //建立File对象用于从SPIFFS中读取文件
		String wifiJsonRead;
  		for(int i=0; i<dataFileRead.size(); i++){
			wifiJsonRead = wifiJsonRead + (char)dataFileRead.read();     //读取文件内容
  		}
		Serial.println(wifiJsonRead);
		Blinker.print(wifiJsonRead);
  		dataFileRead.close();    										 //完成文件读取后关闭文件
		return;
	}
	if(data == "scan wifi" && mode == DEFAULTMODE) {	
		xSemaphoreTake(wifiSemaph, pdMS_TO_TICKS(5000));	 			 //获取信号量
		int wifiNum = WiFi.scanNetworks();								 //扫描WiFI
		xSemaphoreGive(wifiSemaph);						 				 //释放信号量

		StaticJsonDocument<128> SSIDListDoc;
		Blinker.print("Find " + (String)wifiNum + " Network(s)");		
		for(int i = 0; i < wifiNum; ++i) {
			Blinker.print((String)i + ":  " + WiFi.SSID(i));			 //显示WiFI SSID
			Blinker.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Unencrypted" : "Encrypted"));
			SSIDListDoc[i] = WiFi.SSID(i);
		}
		pSSID.reset(new String);
		serializeJson(SSIDListDoc, *pSSID);
		text.print("Please Enter WiFi Number :", "Start from 0");
		mode = CHOOSE_WIFI;
		return;
	}else if(mode == CHOOSE_WIFI) {
		StaticJsonDocument<128> SSIDListDoc;
		deserializeJson(SSIDListDoc, *pSSID);
		
		if(!SPIFFS.begin()){											 //启用SPIFFS
			text.print("SPIFFS Failed to Start!");
		}
  		File dataFileRead = SPIFFS.open(fileName, "r"); 				 //建立File对象用于从SPIFFS中读取文件
		String wifiJsonRead;
  		for(int i=0; i<dataFileRead.size(); i++) {
			wifiJsonRead = wifiJsonRead + (char)dataFileRead.read();     //读取文件内容
  		}
  		dataFileRead.close();    										 //完成文件读取后关闭文件

		DynamicJsonDocument doc(256);
		deserializeJson(doc, wifiJsonRead);								 //解析Json
		int size = doc["SSID"].size();
		if(data.toInt() < 0 || data.toInt() > SSIDListDoc.size() - 1) {
			text.print("Invalid Input", "Please Enter Directive");
			mode = DEFAULTMODE;
			pSSID.reset();
			SPIFFS.end();
			return;
		}
		doc["SSID"][size] = SSIDListDoc[data.toInt()];

		String buffer;
		serializeJson(doc, buffer);
		File dataFileWrite = SPIFFS.open(fileName, "w");				 //建立File对象用于写入文件
		dataFileWrite.print(buffer);									 //文件反写回SPIFFS
		dataFileWrite.close();											 //完成写入后关闭文件
		SPIFFS.end();

		text.print("Setting WiFi :", "Please Enter Password");
		mode = PASSWORD_SETTING;
		pSSID.reset();
		return;
	}
	if(data == "add wifi" && mode == DEFAULTMODE) {
		text.print("Setting WiFi :", "Please Enter SSID");
		mode = SSID_SETTING;
		return;
	}else if(mode == SSID_SETTING) {
		if(!SPIFFS.begin()){											 //启用SPIFFS
			text.print("SPIFFS Failed to Start!");
		}
  		File dataFileRead = SPIFFS.open(fileName, "r"); 				 //建立File对象用于从SPIFFS中读取文件
		String wifiJsonRead;
  		for(int i=0; i<dataFileRead.size(); i++){
			wifiJsonRead = wifiJsonRead + (char)dataFileRead.read();     //读取文件内容
  		}
  		dataFileRead.close();    										 //完成文件读取后关闭文件

		DynamicJsonDocument doc(256);
		deserializeJson(doc, wifiJsonRead);								 //解析Json
		int size = doc["SSID"].size();
		doc["SSID"][size] = data;

		String buffer;
		serializeJson(doc, buffer);
		File dataFileWrite = SPIFFS.open(fileName, "w");				 //建立File对象用于写入文件
		dataFileWrite.print(buffer);									 //文件反写回SPIFFS
		dataFileWrite.close();											 //完成写入后关闭文件
		SPIFFS.end();

		text.print("Setting WiFi :", "Please Enter Password");
		mode = PASSWORD_SETTING;
		return;
	}else if(mode == PASSWORD_SETTING) {
		if(!SPIFFS.begin()){											 //启用SPIFFS
			text.print("SPIFFS Failed to Start!");
		}
  		File dataFileRead = SPIFFS.open(fileName, "r"); 				 //建立File对象用于从SPIFFS中读取文件
		String wifiJsonRead;
  		for(int i=0; i<dataFileRead.size(); i++){
			wifiJsonRead = wifiJsonRead + (char)dataFileRead.read();     //读取文件内容
  		}
  		dataFileRead.close();    										 //完成文件读取后关闭文件

		DynamicJsonDocument doc(256);
		deserializeJson(doc, wifiJsonRead);								 //解析Json
		int size = doc["PASSWORD"].size();
		doc["PASSWORD"][size] = data;

		String buffer;
		serializeJson(doc, buffer);
		File dataFileWrite = SPIFFS.open(fileName, "w");				 //建立File对象用于写入文件
		dataFileWrite.print(buffer);									 //文件反写回SPIFFS
		dataFileWrite.close();											 //完成写入后关闭文件
		SPIFFS.end();

		wifi.addAP(doc["SSID"][size], doc["PASSWORD"][size]);			 //添加AP

		text.print("Setting WiFi :", "Finished!");
		mode = DEFAULTMODE;
		return;
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
			if(weather.temperature() < 10 && weather.temperature() >= 0) {
				iv18.setNowDisplaying(String("TEPR   ") + weather.temperature());	//1位数	
			}else if(weather.temperature() < -10) {
				iv18.setNowDisplaying(String("TEPR "  ) + weather.temperature());	//带符号，占仨位
			}else {
				iv18.setNowDisplaying(String("TEPR  " ) + weather.temperature());	//两位(正常情况下？)
			}
			break;
		case 2:
			iv18.setNowDisplaying(String("RH    ") + weather.humidity());			//应该不太可能出现1位或者3位吧...
			break;
		case 3:
			iv18.setNowDisplaying(weather.weather());								//直接显示气象
			break;
		case 4:
			if(WiFi.status() != WL_CONNECTED) {										//查询WiFi状态
				iv18.setNowDisplaying("NET  OFF");
			}else {
				iv18.setNowDisplaying("NET   ON");
			}
			break;
		case 5:
			if(Blinker.connected()) {
				iv18.setNowDisplaying("BLE  CAT");
			}else {
				iv18.setNowDisplaying("BLE   ON");
			}
			break;
	}
}

void IRAM_ATTR changePage() {
	if(trigged == false) {
		xTaskCreate(resetter, "Resetter", 1024, NULL, 31, NULL); //Resetter,Lower than 1024 may cause some problem
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
	//SPIFFS.format();												 //如果未初始化SPIFFS，请调用一次该函数
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

	if(SPIFFS.begin()){												 //启用SPIFFS
		Serial.println("SPIFFS Started!");
	}else {
		Serial.println("SPIFFS Failed to Start!");
	}
  	File dataFileRead = SPIFFS.open(fileName, "r"); 				 //建立File对象用于从SPIFFS中读取文件
	String wifiJsonRead;
  	for(int i=0; i<dataFileRead.size(); i++){
		wifiJsonRead = wifiJsonRead + (char)dataFileRead.read();     //读取文件内容
  	}
	Serial.println(wifiJsonRead);
  	dataFileRead.close();    										 //完成文件读取后关闭文件
	SPIFFS.end();													 //关闭SPIFFS

	DynamicJsonDocument doc(256);
	deserializeJson(doc, wifiJsonRead);								 //解析Json并依此添加AP
	for(int i = 0; i < doc["SSID"].size(); ++i) {
		wifi.addAP(doc["SSID"][i], doc["PASSWORD"][i]);
	}

	xSemaphoreGive(wifiSemaph);

	WiFi.mode(WIFI_STA);

	Blinker.begin();												 //启动蓝牙	
	Blinker.attachData(dataRead);									 //绑定中断
	xTaskCreate(displayLoop, "dispLoop", 1024 * 1, NULL, 32, NULL);	 //显示进程
	xTaskCreate(getNTPTime , "Regulate", 1024 * 4, NULL, 31, NULL);	 //在线同步时间进程
	xTaskCreate(getWeather , "Weather ", 1024 * 4, NULL, 31, NULL);	 //在线同步天气进程
}

void loop() {                       
	if(Blinker.connected()) {
		Blinker.run();												 //事件处理
		vTaskDelay(pdMS_TO_TICKS( 50));
	}else {
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}