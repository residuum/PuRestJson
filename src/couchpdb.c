#include "couchpdb.h"
#include "couchdb.c"
#include "json.c"

void couchpdb_setup(void) {
	post("CouchPdb: A library for connecting to CouchDB and encoding and decoding JSON data from Puredata.");
	post("(c) Thomas Mayer (Residuum) 2011");
	post("Get the latest source from https://github.com/residuum/CouchPdb");
	post("Found bugs? Report them on the Github page as well.");
	setup_couchdb();
	setup_json_encoder();
	setup_json_decoder();
}

char *remove_backslashes(char *source_string) {
	char *dest = NULL;
	char remove[2] = "\\,";
	size_t len_src = strlen(source_string);
	int found;
	int i = 0;
	int j = 0;

	dest = (char *) malloc(sizeof(char) * len_src + 1);
	if (NULL == dest) {
		printf("Unable to allocate memory\n");
	}

	memset(dest, 0x00,sizeof(char) * len_src + 1 );

	for ( i = 0; i < len_src; i++ ) {
		found = FALSE;
		if (source_string[i] == remove[0] && source_string[i +1] == remove[1]) {
			i++;
			found = TRUE;
		}

		if (FALSE == found)	{
			dest[j] = source_string[i];
		} else {
			dest[j] = ',';
		}
			j++;
	}
	return (dest);
}
