//
// POGSAC: paging.cpp
// POCSAG type interface (NRZ) for outputting messages to the pager
//
// Supported methods (use -D in build flags):
//	- TX_METHOD_SPI: use an SPI transceiver (probably more stable with multitasking on ESP?)
//  - TX_METHOD_BITBANG: use Arduino DigitalWrite() function on a GPIO
//


#include <Arduino.h>
#include "paging.h"
#include "pagers.h"
#include "pocsag.h"

#if defined(TX_METHOD_SPI)
#include "utils.h"
#include <SPI.h>
#include <driver/spi_master.h>

// 
// Seems that we need a bit more precise control over SPI hardware here
// and DMA for performance in multitasking with BLE and other going on,
// so that's why not using the standard SPI Arduino library
// Todo check if it can be used anyway?
// 

/// Interface config of SPI device
spi_device_interface_config_t if_cfg;
/// Interface config of SPI bus
spi_bus_config_t bus_cfg;
/// SPI device handle
spi_device_handle_t handle;

/// Select SPI driver host (either HSPI or VSPI should work)
spi_host_device_t host {HSPI_HOST};
/// Select SPI driver mode (we only use MOSI pin so don't really care)
uint8_t mode {SPI_MODE3};
/// Select DMA channel of SPI, must be 1 or 2
int dma_chan {1};
/// Select max DMA buffer size in BYTES
int max_size {4096};
/// Select SPI baud rate
uint32_t frequency {PAGER_BAUD};
/// DMA buffer
void* dma_buffer;

#endif

static char LOG_TAG[] = "APL_MOD_POCSAG";

/// A pager message
typedef struct paging_queue_item {
	/// Pager capcode
	uint32_t address;
	/// Pager tone code
	uint8_t func;
	/// Message in ASCII
	char* message;
} paging_queue_item_t;

/// Queue of messages to be sent
static QueueHandle_t paging_queue;

/// Task function for main pager I/F loop
extern void paging_task(void*);

void paging_init() {
#if defined(TX_METHOD_BITBANG)

	pinMode(PAGER_GPIO, OUTPUT);
	
#elif defined(TX_METHOD_SPI)
	// Use MOSI at pager pin, don't use other pins
	bus_cfg.sclk_io_num = -1;
	bus_cfg.miso_io_num = -1;
	bus_cfg.mosi_io_num = PAGER_GPIO;
	if_cfg.spics_io_num = -1; //<- CS pin is selected at interface level

	// Set max transfer size in bytes
	bus_cfg.max_transfer_sz = max_size;
	
	// Suitable configuration found experimentally
	if_cfg.mode = mode;
    if_cfg.clock_speed_hz = frequency;
    if_cfg.queue_size = 1;
    if_cfg.flags = SPI_DEVICE_NO_DUMMY;
    if_cfg.pre_cb = NULL;
    if_cfg.post_cb = NULL;
    if_cfg.dummy_bits = 0;
    if_cfg.command_bits = 0;
    if_cfg.address_bits = 0;
    
    // Open the SPI bus and configure
    esp_err_t e = spi_bus_initialize(host, &bus_cfg, dma_chan);
    if (e != ESP_OK) {
        ESP_LOGE(LOG_TAG, "[ERROR] SPI bus initialize failed : %d\n", e);
    }

	// Set a handle on the SPI bus
    e = spi_bus_add_device(host, &if_cfg, &handle);
    if (e != ESP_OK) {
        ESP_LOGE(LOG_TAG, "[ERROR] SPI bus add device failed : %d\n", e);
    }
    
    // Make a DMA buffer for the messages output
    dma_buffer = heap_caps_malloc(max_size, MALLOC_CAP_DMA);
    
#else
	#error "TX method not defined (TX_METHOD_SPI or TX_METHOD_BITBANG)"
#endif

	// Create a queue of pager messages
	paging_queue = xQueueCreate(PAGING_QUEUE_SIZE, sizeof(paging_queue_item_t));
	
	// Start up main loop
	xTaskCreatePinnedToCore(
		paging_task,
		"POCSAG I/F",
		4096, // stack depth
		nullptr, // args
		configMAX_PRIORITIES - 1,
		nullptr, // task handle out
		1 // core
	);
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
	// Prepare a message to be displayed by the pager as ASCII
	ESP_LOGI(LOG_TAG, "Begin transcode: %s", text.c_str());
	transcode_pager_string(&text);
	
	// assuming ascii (single byte) after transcode
	char * strMsg = (char*) malloc(text.length()+1);
	if(strMsg == nullptr) {
		ESP_LOGE(LOG_TAG, "Error allocating message text memory");
	}
	strcpy(strMsg, text.c_str());
	
	ESP_LOGI(LOG_TAG, "End transcode: %s", strMsg);
	
	// Enqueue the message
	paging_queue_item_t message;
	message.address = address;
	message.func = function;
	message.message = strMsg;
	xQueueSend(paging_queue, (void*) &message, 10);
}

void paging_bonk() {
	paging_queue_item_t message;
	
	POCSAG_tx* p_tx;

	// Create a transmission
	p_tx = create_preamble();
	if(p_tx == nullptr) {
		ESP_LOGE(LOG_TAG, "Failed to create TX object");
	}
	
	// Wait for any messages in the message queue
	if(xQueueReceive(paging_queue, (void*)&message, pdMS_TO_TICKS( PAGER_NET_PRESENCE_PREAMBLE_INTERVAL )) == pdTRUE) {
		ESP_LOGI(LOG_TAG, "Task got message... addr=%i func=%i text=%s", message.address, message.func, message.message);
		
		// Add them into the transmission
		if (add_message(p_tx, message.address, message.func, (uint8_t*)message.message, 0) == (-1)) {
			ESP_LOGE(LOG_TAG, "Failed to append message");
		}
		
		// Take care of memory taken up by the message string (malloc'ed in page_message)
		if(message.message != nullptr) {
			free(message.message);
		}
	}
	
#if defined(TX_METHOD_BITBANG)

	// Use bitbang output to send the message
	if (pocsag_out(p_tx, pocsag_out_bit, PAGER_GPIO_INVERSE, 0) == (-1)) {
		ESP_LOGE(LOG_TAG, "Failed to output message bit");
	} else {
		ESP_LOGV(LOG_TAG, "BITBANG sent success");
	}
	
#elif defined(TX_METHOD_SPI)
	
	// Get the full transmission as bytes inside the DMA buffer
	uint32_t got_size = get_cws(p_tx, (uint32_t*) dma_buffer, max_size);
	
#ifdef POCSAG_DUMP_BITS
	// Show transmission buffer
	hexDump((uint8_t*) dma_buffer, got_size);
#endif

	uint32_t* b = (uint32_t*) dma_buffer;
	// Iterate over uint32's for performance reasons
	for(uint32_t p = 0; p < got_size/4; p++) {
		// Invert the bits if necessary
		if(PAGER_GPIO_INVERSE) *b = ~*b;
		// Swap endianness to make sure correct bit order on the wire
		*b = SPI_SWAP_DATA_TX(*b, 32);
		b++;
	}
	
	// Begin an SPI transaction
	spi_transaction_t t;
	t.flags = 0;			  // no flags
    t.length = 8 * got_size;  // set Tx size in bits
    t.tx_buffer = dma_buffer; // Tx from DMA buffer
    t.rx_buffer = nullptr;    // Rx to nowhere
    t.rxlength = 0;			  // don't Rx at all
    
    // Request exclusive SPI access for timing precision
    esp_err_t e = spi_device_acquire_bus(handle, portMAX_DELAY);
    if (e != ESP_OK) {
        ESP_LOGE(LOG_TAG, "[ERROR] SPI device lock failed : %d\n", e);
    } 
    
    // Send data over SPI interface
    e = spi_device_transmit(handle, &t);
    if (e != ESP_OK) {
        ESP_LOGE(LOG_TAG, "[ERROR] SPI device transmit failed : %d\n", e);
    } else {
		ESP_LOGV(LOG_TAG, "SPI sent %d bytes", got_size);
	}

	// Release exclusive access of SPI bus
	spi_device_release_bus(handle);
	
#endif

	// Release transmission memory
	free(p_tx);
}

void paging_task(void* p) {
	while(1) paging_bonk();
}
