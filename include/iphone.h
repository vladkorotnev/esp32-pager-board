#ifndef _IPHONE_H
#define _IPHONE_H

/// Time in ms. to ignore pre-existing notifications after connection
#define IPHONE_CONNECTION_COOLDOWN 5000 
extern void start_ios_ble();

#define CATEGORY_TO_FLAG(x) (1<<x)

#endif
