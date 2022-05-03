#ifndef _PAGERS_H
#define _PAGERS_H

#include <Arduino.h>

#define PAGER_BAUD 1200
#define PAGER_GPIO 5
#define PAGER_GPIO_INVERSE 1

#define PAGER_ADDR 822440
#define PAGER_FUNC 2

extern void transcode_pager_string(String* source);

#endif
