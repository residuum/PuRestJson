#include "purest_json.h"

void purest_json_setup(void) {
	post("PuREST JSON version %s: A library for executing HTTP queries and encoding and decoding JSON data from Puredata.", LIBRARY_VERSION);
	post("(c) Thomas Mayer (Residuum) 2011");
	post("Get the latest source from https://github.com/residuum/PuRestJson");
	rest0x2djson_setup();
	json0x2dencoder_setup();
	json0x2ddecoder_setup();
}
