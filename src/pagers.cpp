#include <Arduino.h>


#ifdef CODE_TABLE_NEC_CYRILLIC

void transcode_pager_string(String* source) {
	source->toUpperCase();
	
	source->replace("А", "A");
	source->replace("Б", "a");
	source->replace("В", "B");
	source->replace("Г", "b");
	source->replace("Д", "d");
	source->replace("Е", "E");
	source->replace("Ё", "e");
	source->replace("Ж", "f");
	source->replace("З", "g");
	source->replace("И", "h");
	source->replace("Й", "i");
	source->replace("К", "K");
	source->replace("Л", "j");
	source->replace("М", "M");
	source->replace("Н", "H");
	source->replace("О", "O");
	source->replace("П", "k");
	source->replace("Р", "P");
	source->replace("С", "C");
	source->replace("Т", "T");
	source->replace("У", "l");
	source->replace("Ф", "m");
	source->replace("Х", "X");
	source->replace("Ц", "n");
	source->replace("Ч", "o");
	source->replace("Ш", "p");
	source->replace("Щ", "q");
	source->replace("Ъ", "r");
	source->replace("Ы", "s");
	source->replace("Ь", "t");
	source->replace("Э", "u");
	source->replace("Ю", "v");
	source->replace("Я", "w");
	
	source->replace("а", "A");
	source->replace("б", "a");
	source->replace("в", "B");
	source->replace("г", "b");
	source->replace("д", "d");
	source->replace("е", "E");
	source->replace("ё", "e");
	source->replace("ж", "f");
	source->replace("з", "g");
	source->replace("и", "h");
	source->replace("й", "i");
	source->replace("к", "K");
	source->replace("л", "j");
	source->replace("м", "M");
	source->replace("н", "H");
	source->replace("о", "O");
	source->replace("п", "k");
	source->replace("р", "P");
	source->replace("с", "C");
	source->replace("т", "T");
	source->replace("у", "l");
	source->replace("ф", "m");
	source->replace("х", "X");
	source->replace("ц", "n");
	source->replace("ч", "o");
	source->replace("ш", "p");
	source->replace("щ", "q");
	source->replace("ъ", "r");
	source->replace("ы", "s");
	source->replace("ь", "t");
	source->replace("э", "u");
	source->replace("ю", "v");
	source->replace("я", "w");
}

#else
#error "Code table not specify"
#endif
