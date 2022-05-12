/**
 * A notification.
 */

// Based on CarWatch project
#ifndef BLE_NOTIFICATION_H_
#define BLE_NOTIFICATION_H_

#include <string>
#include <WString.h> // Arduino string

/**
 * Notification category, based on ANCS values, but could also be used for Android.
 */
typedef enum
{
    CategoryIDOther = 0,
    CategoryIDIncomingCall = 1,
    CategoryIDMissedCall = 2,
    CategoryIDVoicemail = 3,
    CategoryIDSocial = 4,
    CategoryIDSchedule = 5,
    CategoryIDEmail = 6,
    CategoryIDNews = 7,
    CategoryIDHealthAndFitness = 8,
    CategoryIDBusinessAndFinance = 9,
    CategoryIDLocation = 10,
    CategoryIDEntertainment = 11
} NotificationCategory;

namespace ANCS { // Enums specific to ANCS. The others we can reuse for Android

typedef enum
{
    EventIDNotificationAdded = 0,
    EventIDNotificationModified = 1,
    EventIDNotificationRemoved = 2
} event_id_t;

typedef enum
{
    NotificationActionPositive = 0,
    NotificationActionNegative = 1
} NotificationAction;


typedef enum
{
    EventFlagSilent = (1 << 0),
    EventFlagImportant = (1 << 1),
    EventFlagPreExisting = (1 << 2),
    EventFlagPositiveAction = (1 << 3),
    EventFlagNegativeAction = (1 << 4)
} EventFlags;

typedef enum
{
    CommandIDGetNotificationAttributes = 0,
    CommandIDGetAppAttributes = 1,
    CommandIDPerformNotificationAction = 2
} command_id_t;


typedef enum
{
    NotificationAttributeIDAppIdentifier = 0, /**< ie com.facebook.Messenger, or some other app */
    NotificationAttributeIDTitle = 1,    // (Needs to be followed by a 2-bytes max length parameter)
    NotificationAttributeIDSubtitle = 2, // (Needs to be followed by a 2-bytes max length parameter)
    NotificationAttributeIDMessage = 3,  // (Needs to be followed by a 2-bytes max length parameter)
    NotificationAttributeIDMessageSize = 4,
    NotificationAttributeIDDate = 5,
    NotificationAttributeIDPositiveActionLabel = 6,
    NotificationAttributeIDNegativeActionLabel = 7
} notification_attribute_id_t;

};

typedef enum {
    HasBundleID = 1 << 0,
    HasTitle = 1 << 1,
    HasSubtitle = 1 << 2,
    HasMessage = 1 << 3,
    HasDate = 1 << 4
} attrib_reception_flags_t;

#define HAS_ALL_NEEDED_FOR_SHOW (HasBundleID | HasTitle | HasSubtitle | HasMessage)

/**
 * A notification, usable by the caller of the library.
 */
struct Notification {
    std::string title;
    std::string subtitle;
    std::string message;
    std::string type;
	uint32_t eventFlags; /**< Bitfield of ANCS::EventFlags flags. */
    time_t time;
    uint32_t uuid = 0;
    bool showed = false;
    bool isComplete = false;
	NotificationCategory category; /**< If it is a call, social media, email, etc. */
	uint8_t categoryCount; /**< Number of other notifications in this category (ie badge number count). */
    uint8_t receptionFlags;
};

/**
 * C++ strings might be confusing for beginners, so this is the same struct as Notification, but using
 * Arduino strings. We still use Notification for the underlying logic, because of the heap fragmentation issues
 * with Arduino strings.
 */
struct ArduinoNotification {
    String title;
    String subtitle;
    String message;
    String type;
	uint32_t eventFlags; /**< Bitfield of ANCS::EventFlags flags. */
    time_t time;
    uint32_t uuid = 0;
    bool showed = false;
    bool isComplete = false;
	NotificationCategory category; /**< If it is a call, social media, email, etc. */
	uint8_t categoryCount; /**< Number of other notifications in this category (ie badge number count). */
    attrib_reception_flags_t receptionFlags;
	
	ArduinoNotification(const Notification & src) {
		title = String(src.title.c_str());
        subtitle = String(src.subtitle.c_str());
		message = String(src.message.c_str());
		type = String(src.type.c_str());
		eventFlags = src.eventFlags;
		time = src.time;
		uuid = src.uuid;
		showed = src.showed;
		isComplete = src.isComplete;
		category = src.category;
		categoryCount = src.categoryCount;
	}
};

/**
 * Callback for when a notification arrives.
 * @param notification The notification that just arrived.
 */
typedef void (*ble_notification_arrived_t)(const ArduinoNotification * notification, const Notification * rawNotificationData);

/**
 * Callback for when a notification was removed.
 * @param notification The notification that was removed.
 */
typedef void (*ble_notification_removed_t)(const ArduinoNotification * notification, const Notification * rawNotificationData);




#endif
