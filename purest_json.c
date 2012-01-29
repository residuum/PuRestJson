#include "purest_json.h"

void purest_json_setup(void) {
	post("PuREST JSON version %s: A library for executing HTTP queries and encoding and decoding JSON data from Puredata.", LIBRARY_VERSION);
	post("(c) Thomas Mayer (Residuum) 2011");
	post("Get the latest source from https://github.com/residuum/PuRestJson");
	setup_rest0x2djson();
	setup_json0x2dencode();
	setup_json0x2ddecode();
}
