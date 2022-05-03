#ifndef _PAGING_H
#define _PAGING_H

#include <Arduino.h>

extern void paging_init();
extern void page_message(String text, uint32_t address, uint8_t function);

#endif
