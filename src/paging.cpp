#include <Arduino.h>
#include "paging.h"
#include "pagers.h"
#include "pocsag.h"

static char LOG_TAG[] = "APL_MOD_POCSAG";

SemaphoreHandle_t pagingSendMutex = xSemaphoreCreateMutex();

void paging_init() {
	pinMode(PAGER_GPIO, OUTPUT);
	assert(pagingSendMutex);
}

#ifdef POCSAG_DUMP_BITS
#undef PAGER_GPIO_INVERSE
#define PAGER_GPIO_INVERSE 0
	uint8_t cur_dbg_byte = 0x0;
	uint8_t cur_dbg_bit = 0;
	int pocsag_dbg_bit(int bit) {
		Serial.write(bit ? "1" : "0");
		return 0;
	}
#else
	int pocsag_out_bit(int bit) {
		digitalWrite(PAGER_GPIO, bit);
		delayMicroseconds(1000000 / PAGER_BAUD);
		
		return 0;
	}
#endif //POCSAG_DUMP_BITS


void send_message(const char* msg, uint32_t address, uint8_t func) {
	POCSAG_tx* p_tx;
	int invert;
	
	invert = PAGER_GPIO_INVERSE;
	
	ESP_LOGI(LOG_TAG, "=== Begin send ===");
	
	p_tx = create_preamble();
	if(p_tx == nullptr) {
		ESP_LOGE(LOG_TAG, "Failed to create TX object");
	}
	
	if (add_message(p_tx, address, func, (uint8_t*)msg, 0) == (-1)) {
		ESP_LOGE(LOG_TAG, "Failed to append message");
	}
	
	ESP_LOGI(LOG_TAG, "Waiting on send mutex to free up...");
	
	xSemaphoreTake(pagingSendMutex, portMAX_DELAY); 
	
	if (pocsag_out(p_tx, pocsag_out_bit, invert, 0) == (-1)) {
		ESP_LOGE(LOG_TAG, "Failed to output message bit");
	}
	
	xSemaphoreGive(pagingSendMutex);
	
	free(p_tx);
	ESP_LOGI(LOG_TAG, "=== End send ===");
}

void page_message(String text, uint32_t address, uint8_t function) {
	ESP_LOGI(LOG_TAG, "Begin transcode: %s", text.c_str());
	transcode_pager_string(&text);
	ESP_LOGI(LOG_TAG, "End transcode: %s", text.c_str());
	
	send_message(text.c_str(), address, function);
}
