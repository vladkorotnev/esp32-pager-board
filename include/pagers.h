//
// POGSAC: pagers.h
// Configuration of specific pager model quirks
//


#ifndef _PAGERS_H
#define _PAGERS_H

#include <Arduino.h>

/// Time interval (ms) to send preamble if no message was queued to emulate a present network
#define PAGER_NET_PRESENCE_PREAMBLE_INTERVAL 10000 //ms
/// Baud rate for the pager
#define PAGER_BAUD 1200
/// GPIO where the pager will reside
#define PAGER_GPIO 5
/// Whether to invert the pager output
#define PAGER_GPIO_INVERSE 1

/// Pager's capcode (including frame assignment, aka operator code)
#define PAGER_ADDR 822440
/// Pager's function (aka tone code?)
#define PAGER_FUNC 2

/// Transcode a string in-place to be readable by the pager
extern void transcode_pager_string(String* source);

#endif
