//
// POGSAC: iphone.cpp
// Bridge module for ANCS (Apple Notification Center Service) to Pager
//

#include <Arduino.h>
#include "esp32notifications.h"
#include "utils.h"
#include "paging.h"
#include "pagers.h"
#include "iphone.h"

static char LOG_TAG[] = "APL_MOD_IPHONE";

BLENotifications notifications;

// "Ignore" destination for use in bundle-ID filter
static const paging_destination_t IGNORE_DEST = {UINT32_MAX, UINT8_MAX};
#define DEST_IS_BLACKHOLED(dst) (dst.address == IGNORE_DEST.address && dst.func == IGNORE_DEST.func)

// Simple category toggle: used for early filtering
static const uint16_t allowedCategoryBitmask = 0
    | CATEGORY_TO_FLAG(CategoryIDOther)
    | CATEGORY_TO_FLAG(CategoryIDIncomingCall)
    | CATEGORY_TO_FLAG(CategoryIDMissedCall)
    | CATEGORY_TO_FLAG(CategoryIDVoicemail)
    | CATEGORY_TO_FLAG(CategoryIDSocial)
    | CATEGORY_TO_FLAG(CategoryIDSchedule)
    | CATEGORY_TO_FLAG(CategoryIDEmail)
    | CATEGORY_TO_FLAG(CategoryIDNews)
    | CATEGORY_TO_FLAG(CategoryIDHealthAndFitness)
    | CATEGORY_TO_FLAG(CategoryIDBusinessAndFinance)
    | CATEGORY_TO_FLAG(CategoryIDLocation)
    | CATEGORY_TO_FLAG(CategoryIDEntertainment)
    ;
    
// Simple category router
static const paging_destination_t categoryDestinationsTable[] = {
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDOther = 0,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDIncomingCall = 1,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDMissedCall = 2,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDVoicemail = 3,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDSocial = 4,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDSchedule = 5,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDEmail = 6,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDNews = 7,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDHealthAndFitness = 8,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDBusinessAndFinance = 9,
    {PAGER_ADDR, PAGER_FUNC}, // CategoryIDLocation = 10,
    {PAGER_ADDR, PAGER_FUNC}  // CategoryIDEntertainment = 11
};

// Simple bundle ID router, todo use hashmap etc?
typedef struct bundle_id_override { 
    const char * bundleId;
    paging_destination_t destination; 
} bundle_id_override_t;

static bundle_id_override_t bundleOverridesTable[] = {
    // Bundle ID -------------------| New address/func ---------------- |
//    {  "ph.telegra.Telegraph",        {1234, 1}                       }
};

static void lookupBundleIdDest(const char * bundleId, paging_destination_t* rslt) {
    for(size_t i = 0; i < sizeof(bundleOverridesTable)/sizeof(bundle_id_override_t); i++) {
        if(strcmp(bundleId, bundleOverridesTable[i].bundleId) == 0) {
            paging_destination_t new_dst = bundleOverridesTable[i].destination;
            rslt->address = new_dst.address;
            rslt->func = new_dst.func;
            ESP_LOGI(LOG_TAG, "Override found dst={%lu, %i} for bundleId=\"%s\"", rslt->address, rslt->func, bundleId);
            break;
        }
    }
}

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
void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData) {
    // Discard pre-existing notifications altogether
    if((rawNotificationData->eventFlags & ANCS::EventFlags::EventFlagPreExisting) != 0) {
        ESP_LOGI(LOG_TAG, "Received PRE-EXISTING notification 0x%x, discard", rawNotificationData->uuid);
        return;
    } 
    
    // Discard silent notifications altogether
    if((rawNotificationData->eventFlags & ANCS::EventFlags::EventFlagSilent) != 0) {
        ESP_LOGI(LOG_TAG, "Received SILENT notification 0x%x, discard", rawNotificationData->uuid);
        return;
    }
    
    // Discard all but some categories altogether
    if((allowedCategoryBitmask & CATEGORY_TO_FLAG(rawNotificationData->category)) == 0) {
        ESP_LOGI(LOG_TAG, "Received notification 0x%x with category %i not allowed in bitmask 0x%04x, discard", rawNotificationData->uuid, rawNotificationData->category, allowedCategoryBitmask);
        return;
    }
        
    if(notification == nullptr) {
        // Only received basic info at this point, 
        // request full if not pre-existing and TODO: qualified to show
        
        if((rawNotificationData->receptionFlags & HasBundleID) == 0) {
            ESP_LOGI(LOG_TAG, "Received PARTIAL notification 0x%x, bundle ID = ??? (requesting)", rawNotificationData->uuid);
            notifications.requestBundleId(rawNotificationData->uuid);
            
            return;
        } else {
            ESP_LOGI(LOG_TAG, "Received PARTIAL notification 0x%x, bundle ID = %s", rawNotificationData->uuid, rawNotificationData->type.c_str());
            paging_destination_t def = {0};
            
            lookupBundleIdDest(rawNotificationData->type.c_str(), &def);
            
            if(DEST_IS_BLACKHOLED(def)) {
                ESP_LOGI(LOG_TAG, "Bundle ID %s is blackholed, discard", rawNotificationData->type.c_str());
            } else {
                notifications.requestMoreInfo(rawNotificationData->uuid);
            }
            
            return;
        }
    }
    
    // Received full info at this point
    
    paging_destination_t dest = categoryDestinationsTable[notification->category];
    
    ESP_LOGI(LOG_TAG, "Received notification 0x%x (category %i)", notification->uuid, notification->category);

    lookupBundleIdDest(rawNotificationData->type.c_str(), &dest);
    
    auto msgBuilder = "" + notification->message;

    if(!notification->title.isEmpty() && !notification->subtitle.isEmpty()) {
        msgBuilder = notification->title + ", " + notification->subtitle + ": " + msgBuilder;
    } else if(!notification->title.isEmpty() && notification->subtitle.isEmpty()) {
        msgBuilder = notification->title + ": " + msgBuilder;
    } else if(notification->title.isEmpty() && !notification->subtitle.isEmpty()) {
        msgBuilder = notification->subtitle + ": " + msgBuilder;
    } 
    
    String msgTotal = String(msgBuilder);
    
    page_message(msgTotal, dest);
}


// A notification was cleared
void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData) {
     ESP_LOGV(LOG_TAG, "Cleared notification 0x%x", notification->uuid);
}


void start_ios_ble() {
    String name = String("POGSAC")+getChipId();
    notifications.setConnectionStateChangedCallback(onBLEStateChanged);
    notifications.setNotificationCallback(onNotificationArrived);
    notifications.setRemovedCallback(onNotificationRemoved);
    notifications.begin(name.c_str());
    ESP_LOGI(LOG_TAG, "Brought up BLE as %s", name.c_str());
}

