# ESP32-Pager-Board

**Currently WIP.**

The objective of the project is to provide a simple ESP32-based
drop-in board to replace a standard POCSAG-pager's baseband.

## Hardware how-to

TBD

TL;DR: Just solder it in place of your pager's modem NRZ output and you're
good to go.

## Software how-to

### Android

TBD (anyone has one of these things and wanna try?)

### iOS

Build with the `-DFOR_IPHONE` build flag (`-DTX_METHOD_SPI` is also recommended for stability of output timing).

## Credits

* [Smartphone-Companions/ESP32-ANCS-Notifications](https://github.com/Smartphone-Companions/ESP32-ANCS-Notifications) — Apple ANCS integration module (local copy to fix some build issues)
* [avk-sw/pocsag2sdr](https://github.com/avk-sw/pocsag2sdr) — basis of POCSAG encoding module (local copy to fix some logic issues)
* [goutsune@github](https://github.com/goutsune) — kick my ass to start implementing the idea I had ever since 2017
