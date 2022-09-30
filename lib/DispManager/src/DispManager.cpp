#include "DispManager.h"
#include "freertos/task.h"

DispManager::DispManager(IV18* iv18T) {
	iv18 = iv18T;
}
void DispManager::bind(IV18* iv18T) {
	iv18 = iv18T;
}

void DispManager::manageLoop() {
	while(1) {
		String time;
		time = time + timeNow.Hour() + timeNow.Minute() + timeNow.Second();

		iv18->setNowDisplaying("HELLO");
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void DispManager::setTime(RtcDateTime time) {
	timeNow = time;
}