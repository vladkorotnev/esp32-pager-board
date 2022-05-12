#include <Arduino.h>
#include "paging.h"
#include "pagers.h"
#include <esp_wifi.h>
#include "driver/adc.h"
#include "soc/rtc.h"

#ifdef FOR_IPHONE
#include "iphone.h"
#endif //FOR_IPHONE

void setup() {
    // Disable useless hardware right away
    esp_wifi_stop(); // No need for WiFi
    adc_power_off(); // No need for ADC (for now?)
    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
    
    // Set up serial for logs
    Serial.begin(115200);
    
    // Set up paging I/F
    paging_init();
    
#ifdef FOR_IPHONE
    // Set up iOS ANCS module
    start_ios_ble();
#endif

#ifdef DUMMY
    // Send a dummy message
    paging_destination_t dst;
    dst.address = PAGER_ADDR;
    dst.func = PAGER_FUNC;
    page_message("MESSAGE TEXT", dst);
#endif

    vTaskDelete(NULL); // Get rid of setup() and loop() task
}

void loop() {}
