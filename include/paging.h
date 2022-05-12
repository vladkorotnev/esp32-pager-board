//
// POGSAC: paging.h
// Functions for interfacing with the pager
//

#ifndef _PAGING_H
#define _PAGING_H

#include <Arduino.h>

/// Max messages in Tx queue
#define PAGING_QUEUE_SIZE 10

/// Initialize the pager interface. Must be called before any other function.
extern void paging_init();

/// A pager message
typedef struct paging_destination {
    /// Pager capcode
    uint32_t address;
    /// Pager tone code
    uint8_t func;
} paging_destination_t;


/// Put a message in the queue to be sent next time the interface is available.
extern void page_message(String text, paging_destination_t recipient);

#endif
