// Based on the ANCS work of https://github.com/S-March and the CarWatch project

#include "ble_security.h"
#include "ancs_ble_client.h"
#include "ancs_notification_queue.h"

#include "BLEAddress.h"
#include "BLEDevice.h"
#include "BLEClient.h"
#include "BLEUtils.h"
#include "BLE2902.h"

#include "esp_log.h"

#include <Arduino.h> // Only for development

static char LOG_TAG[] = "ANCSBLEClient";

// Fixed service IDs for the Apple ANCS service
const BLEUUID notificationSourceCharacteristicUUID("9FBF120D-6301-42D9-8C58-25E699A21DBD");
const BLEUUID controlPointCharacteristicUUID("69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9");
const BLEUUID dataSourceCharacteristicUUID("22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB");
const BLEUUID ancsServiceUUID("7905F431-B5CE-4E99-A40F-4B1E122D00D0");


static ANCSBLEClient * sharedInstance;

static void dataSourceNotifyCallback(
  BLERemoteCharacteristic* pDataSourceCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
	 ESP_LOGD(LOG_TAG, "dataSourceNotifyCallback");
	sharedInstance->onDataSourceNotify(pDataSourceCharacteristic, pData, length, isNotify);
}


static void notificationSourceNotifyCallback(
  BLERemoteCharacteristic* pNotificationSourceCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
	ESP_LOGD(LOG_TAG, "notificationSourceNotifyCallback");
	
	sharedInstance->onNotificationSourceNotify(pNotificationSourceCharacteristic, pData, length, isNotify);
}


ANCSBLEClient::ANCSBLEClient()
	: notificationCB(nullptr)
	, removedCB(nullptr)
	, pControlPointCharacteristic(nullptr)
{
	assert(sharedInstance == nullptr);
	sharedInstance = this;  
	notificationQueue = new ANCSNotificationQueue();
}


ANCSBLEClient::~ANCSBLEClient() {
	sharedInstance = nullptr;
}

void ANCSBLEClient::startClientTask(void * params) {
	ESP_LOGD(LOG_TAG, "Starting client");
		const BLEAddress* address = (BLEAddress*)params;
		sharedInstance->setup(address);
        
        vTaskDelete(NULL);
}

// In task due to a deadlock with BLE callback, todo figure out?
void ANCSBLEClient::requestFullInfoTask(void * params) {
	ESP_LOGD(LOG_TAG, "Requesting full info for show");
    uint32_t uuid = (uint32_t)params;
    
    sharedInstance->requestInfo(uuid, ANCS::NotificationAttributeIDTitle);
    sharedInstance->requestInfo(uuid, ANCS::NotificationAttributeIDSubtitle);
    sharedInstance->requestInfo(uuid, ANCS::NotificationAttributeIDMessage);
    
    vTaskDelete(NULL);
}

// In task due to a deadlock with BLE callback, todo figure out?
void ANCSBLEClient::requestAppIdTask(void * params) {
	ESP_LOGD(LOG_TAG, "Requesting bundle id");
    uint32_t uuid = (uint32_t)params;
    
    sharedInstance->requestInfo(uuid, ANCS::NotificationAttributeIDAppIdentifier);
    
    vTaskDelete(NULL);
}

void ANCSBLEClient::setup(const BLEAddress * address) {
	BLEClient*  pClient  = BLEDevice::createClient();
	BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

	BLESecurity *pSecurity = new BLESecurity();
	pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
	pSecurity->setCapability(ESP_IO_CAP_IO);
	pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
	// Connect to the remove BLE Server.
	pClient->connect(*address, BLE_ADDR_TYPE_RANDOM);

	/** BEGIN ANCS SERVICE **/
	// Obtain a reference to the service we are after in the remote BLE server.
	BLERemoteService* pAncsService = pClient->getService(ancsServiceUUID);
	if (pAncsService == nullptr) {
		ESP_LOGW(LOG_TAG, "Failed to find service UUID ancsServiceUUID.");
		return;
	}
	// Obtain a reference to the characteristic in the service of the remote BLE server.
	BLERemoteCharacteristic* pNotificationSourceCharacteristic = pAncsService->getCharacteristic(notificationSourceCharacteristicUUID);
	if (pNotificationSourceCharacteristic == nullptr) {
		ESP_LOGW(LOG_TAG, "Failed to find characteristic UUID notificationSourceCharacteristicUUID");
		return;
	}        
	// Obtain a reference to the characteristic in the service of the remote BLE server.
	
	pControlPointCharacteristic = pAncsService->getCharacteristic(controlPointCharacteristicUUID);
	if (pControlPointCharacteristic == nullptr) {
		ESP_LOGW(LOG_TAG, "Failed to find characteristic UUID: controlPointCharacteristicUUID");
		return;
	}        
	// Obtain a reference to the characteristic in the service of the remote BLE server.
	BLERemoteCharacteristic* pDataSourceCharacteristic = pAncsService->getCharacteristic(dataSourceCharacteristicUUID);
	if (pDataSourceCharacteristic == nullptr) {
		ESP_LOGW(LOG_TAG, "Failed to find characteristic UUID dataSourceCharacteristicUUID");
		return;
	}        
	const uint8_t v[]={0x1,0x0};
	pDataSourceCharacteristic->registerForNotify(dataSourceNotifyCallback);
	pDataSourceCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)v,2,true);
	pNotificationSourceCharacteristic->registerForNotify(notificationSourceNotifyCallback);
	pNotificationSourceCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)v,2,true);
}

BLEUUID ANCSBLEClient::getAncsServiceUUID() {
	return ancsServiceUUID;
}


void ANCSBLEClient::setNotificationArrivedCallback(ble_notification_arrived_t cbNotification) {
	notificationCB = cbNotification;
}


void ANCSBLEClient::setNotificationRemovedCallback(ble_notification_removed_t cbNotification) {
	removedCB = cbNotification;
}


void ANCSBLEClient::onDataSourceNotify(
  BLERemoteCharacteristic* pNotificationSourceCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
	
	if(pData[0] == ANCS::CommandIDGetNotificationAttributes) {
		std::string message;
	
		uint32_t messageId = pData[4];
		messageId = messageId << 8 | pData[3];
		messageId = messageId << 16 | pData[2];
		messageId = messageId << 24 | pData[1];

		for (int i = 8; i < length; i++)
		{
		  message += (char)pData[i];
		}

		ESP_LOGD(LOG_TAG, "ID: %d raw message: %s type==%d", messageId, message.c_str(), pData[5]);

		Notification * notification = notificationQueue->getNotification(messageId);

		  switch (pData[5])
		  {
			case ANCS::NotificationAttributeIDAppIdentifier:
				notification->type = message;
                notification->receptionFlags |= HasBundleID;
				ESP_LOGD(LOG_TAG, "got type: %s", message.c_str());
                notificationCB(nullptr, notification);
				break;
			case ANCS::NotificationAttributeIDTitle:
				notification->title = message;
                notification->receptionFlags |= HasTitle;
				ESP_LOGD(LOG_TAG, "got title: %s", message.c_str());
				break;
            case ANCS::NotificationAttributeIDSubtitle:
				notification->subtitle = message;
                notification->receptionFlags |= HasSubtitle;
				ESP_LOGD(LOG_TAG, "got subtitle: %s", message.c_str());
				break;
			case ANCS::NotificationAttributeIDMessage:
				notification->message = message;
                notification->receptionFlags |= HasMessage;
				ESP_LOGD(LOG_TAG, "got message: %s", message.c_str());
				break;
		  }
		  if ((notification->receptionFlags & HAS_ALL_NEEDED_FOR_SHOW) == HAS_ALL_NEEDED_FOR_SHOW) {
			if (notificationCB && notification->isComplete == false) {
				ESP_LOGI(LOG_TAG, "got a full notification: %s - %s", notification->title.c_str(), notification->message.c_str());
				const ArduinoNotification arduinoNotification = ArduinoNotification(*notification);
				notificationCB(&arduinoNotification, notification);
			}
			notification->isComplete = true;
		  }
	}
}

bool ANCSBLEClient::isIncomingCall(const Notification & notification) const {
	// @todo detect if it is a FaceTime or call by app? Or is category sufficient?
	return notification.category == CategoryIDIncomingCall;
}
	
void ANCSBLEClient::onNotificationSourceNotify(
	  BLERemoteCharacteristic *pNotificationSourceCharacteristic,
	  uint8_t *pData,
	  size_t length,
	  bool isNotify) {
	  uint32_t messageId;

	  messageId = pData[7];
	  messageId = messageId << 8 | pData[6];
	  messageId = messageId << 16 | pData[5];
	  messageId = messageId << 24 | pData[4];

	if (pData[0] == ANCS::EventIDNotificationRemoved) {
		ESP_LOGI(LOG_TAG, "notification removed: %d", messageId);

		Notification * notification = notificationQueue->getNotification(messageId);

		if (isIncomingCall(*notification))
		{
		  notificationQueue->addNotification(messageId, *notification, false);
		  notificationQueue->removeCallNotification();
		}
		
		if (removedCB) {
			const ArduinoNotification arduinoNotification = ArduinoNotification(*notification);
			removedCB(&arduinoNotification, notification);
		}
	}
	else if (pData[0] == ANCS::EventIDNotificationAdded) {
		ESP_LOGI(LOG_TAG, "notification added, type: %d", pData[2]);
		Notification pending;
		pending.uuid = messageId;
		pending.eventFlags = pData[1];
		pending.category = NotificationCategory(pData[2]);
		pending.categoryCount = pData[3]; 
        if(!notificationQueue->contains(pending.uuid)) {
            notificationQueue->addNotification(pending.uuid, pending, isIncomingCall(pending));
        }
		notificationCB(nullptr, &pending);
	}
}


void ANCSBLEClient::performAction(uint32_t notifyUUID, uint8_t actionID) {
	
  uint8_t uuid[4];
  uuid[0] = notifyUUID;
  uuid[1] = notifyUUID >> 8;
  uuid[2] = notifyUUID >> 16;
  uuid[3] = notifyUUID >> 24;
	
  const uint8_t vPerformAction[] = {ANCS::CommandIDPerformNotificationAction, uuid[0], uuid[1], uuid[2], uuid[3], actionID};
  pControlPointCharacteristic->writeValue((uint8_t *)vPerformAction, (sizeof(vPerformAction)/sizeof(vPerformAction[0])), true);

}

void ANCSBLEClient::requestInfo(uint32_t notifyUUID, ANCS::notification_attribute_id_t attribute) {
    ESP_LOGD(LOG_TAG, "requestInfo: notification=0x%x attr=%i", notifyUUID, attribute);
    uint8_t uuid[4];
    uuid[0] = notifyUUID;
    uuid[1] = notifyUUID >> 8;
    uuid[2] = notifyUUID >> 16;
    uuid[3] = notifyUUID >> 24;
    
    switch(attribute) {
        case ANCS::NotificationAttributeIDAppIdentifier:
        case ANCS::NotificationAttributeIDDate:
            { // Fixed-size attributes
                const uint8_t cmd[] = {ANCS::CommandIDGetNotificationAttributes, uuid[0], uuid[1], uuid[2], uuid[3], attribute};
                pControlPointCharacteristic->writeValue((uint8_t *)cmd, 6, true);
            }
            break;
            
        case ANCS::NotificationAttributeIDTitle:
        case ANCS::NotificationAttributeIDSubtitle:
        case ANCS::NotificationAttributeIDMessage:
            { // Dynamic size attributes
                const uint8_t cmd[] = {ANCS::CommandIDGetNotificationAttributes, uuid[0], uuid[1], uuid[2], uuid[3], attribute, 0x0, 0x10};
                pControlPointCharacteristic->writeValue((uint8_t *)cmd, 8, true);
            }
            break;
            
        default:
            ESP_LOGE(LOG_TAG, "[FATAL] Attribute %i not supported", attribute);
            return;
    }
}
