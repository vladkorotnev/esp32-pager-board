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
}

void loop() {
	 // because LOOP priority is better than FreeRTOS task,
	 // bitbang is done within this function that is called from loop
	 // instead of setting up a separate task (until I figure out how to)
	paging_bonk();
}
