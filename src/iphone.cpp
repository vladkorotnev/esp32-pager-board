#include <Arduino.h>
#include "esp32notifications.h"
#include "utils.h"
#include "paging.h"
#include "pagers.h"

static char LOG_TAG[] = "APL_MOD_IPHONE";

BLENotifications notifications;

// This callback will be called when a Bluetooth LE connection is made or broken.
// You can update the ESP 32's UI or take other action here.
void onBLEStateChanged(BLENotifications::State state) {
  switch(state) {
      case BLENotifications::StateConnected:
          ESP_LOGI(LOG_TAG, "StateConnected"); 
          break;

      case BLENotifications::StateDisconnected:
          ESP_LOGI(LOG_TAG, "StateDisconnected"); 
          notifications.startAdvertising(); 
          break; 
  }
}


// A notification arrived from the mobile device, ie a social media notification or incoming call.
// parameters:
//  - notification: an Arduino-friendly structure containing notification information. Do not keep a
//                  pointer to this data - it will be destroyed after this function.
//  - rawNotificationData: a pointer to the underlying data. It contains the same information, but is
//                         not beginner-friendly. For advanced use-cases.
void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData) {
	ESP_LOGI(LOG_TAG, "Received notification 0x%x (category %i)", notification->uuid, notification->category);
			
			// todo: routing based on app or category
		uint32_t addr = PAGER_ADDR; 
		uint8_t func = PAGER_FUNC;
		
		String msgTotal = String(notification->title + ":" + notification->message);
		page_message(msgTotal, addr, func);
}


// A notification was cleared
void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData) {
     ESP_LOGI(LOG_TAG, "Cleared notification 0x%x", notification->uuid);
}


void start_ios_ble() {
	String name = String("POGSAC")+getChipId();
    notifications.setConnectionStateChangedCallback(onBLEStateChanged);
    notifications.setNotificationCallback(onNotificationArrived);
    notifications.setRemovedCallback(onNotificationRemoved);
    notifications.begin(name.c_str());
    ESP_LOGI(LOG_TAG, "Brought up as %s", name.c_str());
}

