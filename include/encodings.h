//
// POGSAC: encodings.h
// Configuration of specific pager model encodings
//

#include "pagers.h"

#ifdef CODE_TABLE_NEC_CYRILLIC
// Transcode table for NEC 21A MAXIMA Cyrillic pager, found experimentally
// Also works with TRULY SUPER VISOR pager
bool CODE_TABLE_WANTS_UPPERCASE = true;
pager_code_table_t CODE_TABLE = {
    {"А", "A"},     {"а", "A"},
    {"Б", "a"},     {"б", "a"},
    {"В", "B"},     {"в", "B"},
    {"Г", "b"},     {"г", "b"},
    {"Д", "d"},     {"д", "d"},
    {"Е", "E"},     {"е", "E"},
    {"Ё", "e"},     {"ё", "e"},
    {"Ж", "f"},     {"ж", "f"},
    {"З", "g"},     {"з", "g"},
    {"И", "h"},     {"и", "h"},
    {"Й", "i"},     {"й", "i"},
    {"К", "K"},     {"к", "K"},
    {"Л", "j"},     {"л", "j"},
    {"М", "M"},     {"м", "M"},
    {"Н", "H"},     {"н", "H"},
    {"О", "O"},     {"о", "O"},
    {"П", "k"},     {"п", "k"},
    {"Р", "P"},     {"р", "P"},
    {"С", "C"},     {"с", "C"},
    {"Т", "T"},     {"т", "T"},
    {"У", "l"},     {"у", "l"},
    {"Ф", "m"},     {"ф", "m"},
    {"Х", "X"},     {"х", "X"},
    {"Ц", "n"},     {"ц", "n"},
    {"Ч", "o"},     {"ч", "o"},
    {"Ш", "p"},     {"ш", "p"},
    {"Щ", "q"},     {"щ", "q"},
    {"Ъ", "r"},     {"ъ", "r"},
    {"Ы", "s"},     {"ы", "s"},
    {"Ь", "t"},     {"ь", "t"},
    {"Э", "u"},     {"э", "u"},
    {"Ю", "v"},     {"ю", "v"},
    {"Я", "w"},     {"я", "w"}
};
#else
#warning "Pager code table not specified"
pager_code_table_t CODE_TABLE = {};
bool CODE_TABLE_WANTS_UPPERCASE = false;
#endif
