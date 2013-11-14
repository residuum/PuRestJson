#include "purest_json.h"

void purest_json_setup(void) {
	post("PuREST JSON version %s: A library for executing HTTP queries and encoding and decoding JSON data from Puredata.", PUREST_JSON_VERSION);
	post("(c) Thomas Mayer (Residuum) 2013");
	post("Get the latest source from https://github.com/residuum/PuRestJson");
	post("Website: http://ix.residuum.org/pd/purest_json.html");
	post("Report bugs to: purest-json-bugs@ix.residuum.org");
	rest_setup();
	oauth_setup();
	json_encode_setup();
	json_decode_setup();
	urlparams_setup();
}
