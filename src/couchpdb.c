#include "couchpdb.h"
#include "couchdb.c"
#include "json.c"

void couchpdb_setup(void) {
	setup_couchdb();
	setup_json_encoder();
}
