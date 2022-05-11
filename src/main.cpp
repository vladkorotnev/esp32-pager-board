#include <Arduino.h>
#include "paging.h"
#include "pagers.h"

#ifdef FOR_IPHONE
#include "iphone.h"
#endif //FOR_IPHONE

void setup() {
	Serial.begin(115200);
	
	paging_init();
    
#ifdef FOR_IPHONE
	start_ios_ble();
#endif

#ifdef DUMMY
	page_message("MESSAGE TEXT", PAGER_ADDR, PAGER_FUNC);
#endif

	vTaskDelete(NULL); // Get rid of setup() and loop() task
}

void loop() {}
