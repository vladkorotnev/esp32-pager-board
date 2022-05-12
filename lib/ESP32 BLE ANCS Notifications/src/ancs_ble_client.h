#ifndef ANCS_BLE_CLIENT_H_
#define ANCS_BLE_CLIENT_H_

#include "ble_notification.h"

#include "Arduino.h" // For asynchronous tasks

class BLEAddress;
class BLERemoteCharacteristic;
class ANCSNotificationQueue;


#include "BLEUUID.h"

/**
 * Internal module for creating and managing an ANCS client connection singleton.
 * This needs to run asynchronously as a FreeRTOS task.
 */
class ANCSBLEClient {
public:
   /**
    * Become a BLE client to a remote BLE server. Pass in address of the server.
    */
	ANCSBLEClient();
	virtual ~ANCSBLEClient();
	void setNotificationArrivedCallback(ble_notification_arrived_t cbNotification);
	void setNotificationRemovedCallback(ble_notification_removed_t cbNotification);

	void performAction(uint32_t notifyUUID, uint8_t actionID);

public:
	static BLEUUID getAncsServiceUUID(); // To be able to advertise it
	
	/**
	 * Start the FreeRTOS task that handles the client connection.
	 */
	static void startClientTask(void *data);
    static void sharedSetup(const BLEAddress * address);
    static void requestFullInfoTask(void * params);
    static void requestAppIdTask(void * params);
	
public:
	xTaskHandle clientTaskHandle;
	
	void onDataSourceNotify(BLERemoteCharacteristic*, uint8_t*, size_t, bool);
	void onNotificationSourceNotify(BLERemoteCharacteristic*, uint8_t*, size_t, bool);
	
private:
    void requestInfo(uint32_t notifyUUID, ANCS::notification_attribute_id_t attribute);
	void setup(const BLEAddress*);
	
	/**
	 * Check if this notification is a call event, by checking the triggering app type.
	 * @return true if this is a an incoming call.
	 */
	bool isIncomingCall(const Notification & notification) const;
	
	ANCSNotificationQueue * notificationQueue;
	
	ble_notification_arrived_t notificationCB;
	ble_notification_removed_t removedCB;
	
	class BLERemoteCharacteristic* pControlPointCharacteristic;
};

#endif // ANCS_BLE_CLIENT_H_
