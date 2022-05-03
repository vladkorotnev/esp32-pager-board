#include <Arduino.h>
#include "paging.h"
#include "pagers.h"
#include "pocsag.h"
#include "esp_task_wdt.h"

static char LOG_TAG[] = "APL_MOD_POCSAG";

typedef struct paging_queue_item {
	uint32_t address;
	uint8_t func;
	char* message;
} paging_queue_item_t;

static QueueHandle_t paging_queue;

void paging_init() {
	pinMode(PAGER_GPIO, OUTPUT);
	paging_queue = xQueueCreate(10, sizeof(paging_queue_item_t));
}


#ifdef POCSAG_DUMP_BITS
	uint8_t cur_dbg_byte = 0x0;
	uint8_t cur_dbg_bit = 0;
	int pocsag_out_bit(int bit) {
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


void page_message(String text, uint32_t address, uint8_t function) {
	ESP_LOGI(LOG_TAG, "Begin transcode: %s", text.c_str());
	transcode_pager_string(&text);
	
	// assuming ascii (single byte) after transcode
	char * strMsg = (char*) malloc(text.length()+1);
	if(strMsg == nullptr) {
		ESP_LOGE(LOG_TAG, "Error allocating message text memory");
	}
	strcpy(strMsg, text.c_str());
	
	ESP_LOGI(LOG_TAG, "End transcode: %s", strMsg);
	
	paging_queue_item_t message;
	message.address = address;
	message.func = function;
	message.message = strMsg;
	xQueueSend(paging_queue, (void*) &message, 10);
}

void paging_bonk() {
	paging_queue_item_t message;
	
	int invert;
	
	invert = PAGER_GPIO_INVERSE;
	
	POCSAG_tx* p_tx;

	p_tx = create_preamble();
	if(p_tx == nullptr) {
		ESP_LOGE(LOG_TAG, "Failed to create TX object");
	}

	if(xQueueReceive(paging_queue, (void*)&message, pdMS_TO_TICKS( PAGER_NET_PRESENCE_PREAMBLE_INTERVAL )) == pdTRUE) {
		ESP_LOGI(LOG_TAG, "Task got message... addr=%i func=%i text=%s", message.address, message.func, message.message);
		
		if (add_message(p_tx, message.address, message.func, (uint8_t*)message.message, 0) == (-1)) {
			ESP_LOGE(LOG_TAG, "Failed to append message");
		}
		
		if(message.message != nullptr) {
			free(message.message);
		}
	}
	
	if (pocsag_out(p_tx, pocsag_out_bit, invert, 0) == (-1)) {
		ESP_LOGE(LOG_TAG, "Failed to output message bit");
	}
	
	free(p_tx);
}

