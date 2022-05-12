//
// POGSAC: pagers.h
// Configuration of specific pager model quirks
//

// TODO: Optimize and rework transcode mechanism

#include <Arduino.h>
#include "encodings.h"

void transcode_pager_string(String* source) {
    if(CODE_TABLE_WANTS_UPPERCASE)
        source->toUpperCase();
    
    for(int i = 0; i < sizeof(CODE_TABLE)/sizeof(substitution_t); i++) 
        source->replace(CODE_TABLE[i].first, CODE_TABLE[i].second);
}
