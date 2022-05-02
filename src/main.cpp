#include <Arduino.h>
#include "pocsag.h"

static char LOG_TAG[] = "POGSAC";
const int out_pin = 5;
#define BAUD 1200

#ifdef FOR_IPHONE
#include "esp32notifications.h"
BLENotifications notifications;
// Holds the incoming call's ID number, or zero if no notification
uint32_t incomingCallNotificationUUID;

// This callback will be called when a Bluetooth LE connection is made or broken.
// You can update the ESP 32's UI or take other action here.
void onBLEStateChanged(BLENotifications::State state) {
  switch(state) {
      case BLENotifications::StateConnected:
          Serial.println("StateConnected - connected to a phone or tablet"); 
          break;

      case BLENotifications::StateDisconnected:
          Serial.println("StateDisconnected - disconnected from a phone or tablet"); 
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
    Serial.print("Got notification: ");   
    Serial.println(notification->title); // The title, ie name of who sent the message
    Serial.println(notification->message); // The detail, ie "be home for dinner at 7".
    Serial.println(notification->type);  // Which app sent it
    Serial.println(notifications.getNotificationCategoryDescription(notification->category));  // ie "social media"
    Serial.println(notification->categoryCount); // How may other notifications are there from this app (ie badge number)
    if (notification->category == CategoryIDIncomingCall) {
		// If this is an incoming call, store it so that we can later send a user action.
        incomingCallNotificationUUID = notification->uuid;
        Serial.println("--- INCOMING CALL: PRESS A TO ACCEPT, C TO REJECT ---"); 
    }
    else {
        incomingCallNotificationUUID = 0; // Make invalid - no incoming call
    }
}


// A notification was cleared
void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData) {
     Serial.print("Removed notification: ");   
     Serial.println(notification->title);
     Serial.println(notification->message);
     Serial.println(notification->type);  
}

#endif //FOR_IPHONE

String getChipId() {
  uint64_t macAddress = ESP.getEfuseMac();
  uint32_t id         = 0;

  for (int i = 0; i < 17; i = i + 8) {
    id |= ((macAddress >> (40 - i)) & 0xff) << i;
  }
  return String(id, HEX);
}

int pocsag_out_bit(int bit) {
	digitalWrite(out_pin, bit);
	delayMicroseconds(1000000 / BAUD);
	
	return 0; //OK
}

uint8_t cur_dbg_byte = 0x0;
uint8_t cur_dbg_bit = 0;
int pocsag_dbg_bit(int bit) {
	Serial.write(bit ? "1" : "0");
	return 0;
}

void send_message() {
	POCSAG_tx* p_tx;
	uint32_t cap_code, func;
	const char *msg = "HELLO FROM ESP 32";
	int isNum;
	int invert;
	
	cap_code = 102805; //102805, fn=0 -> Super Visor
	func = 0;
	isNum = 0;
	invert = 0;
	
	ESP_LOGI(LOG_TAG, "=== Begin send ===");
	
	p_tx = create_preamble();
	if(p_tx == nullptr) {
		ESP_LOGE(LOG_TAG, "Failed to create TX object");
	}
	
	if (add_message(p_tx, cap_code, func, (uint8_t*)msg, isNum) == (-1)) {
		ESP_LOGE(LOG_TAG, "Failed to append message");
	}
	
	if (pocsag_out(p_tx, pocsag_dbg_bit, invert, 0) == (-1)) {
		ESP_LOGE(LOG_TAG, "Failed to output message bit");
	}
	
	free(p_tx);
	Serial.println();
	ESP_LOGI(LOG_TAG, "=== End send ===");
}

void setup() {
	Serial.begin(115200);
	
	pinMode(out_pin, OUTPUT);
    
#ifdef FOR_IPHONE
	String name = String("POGSAC")+getChipId();
    notifications.setConnectionStateChangedCallback(onBLEStateChanged);
    notifications.setNotificationCallback(onNotificationArrived);
    notifications.setRemovedCallback(onNotificationRemoved);
    notifications.begin(name.c_str());
#endif
}

void loop() {
	send_message();
	delay(5000);
}
