#include "purest_json.h"
#include "rest.c"
#include "json.c"

void purest_json_setup(void) {
	post("PuREST JSON version %s: A library for executing HTTP queries and encoding and decoding JSON data from Puredata.", LIBRARY_VERSION);
	post("(c) Thomas Mayer (Residuum) 2011");
	post("Get the latest source from https://github.com/residuum/PuRestJson");
	setup_rest();
	setup_json_encoder();
	setup_json_decoder();
}

char *remove_backslashes(char *source_string) {
	char *dest = NULL;
	char remove[2] = "\\,";
	size_t len_src = strlen(source_string);
	int found;
	unsigned int i = 0;
	int j = 0;

	dest = (char *) calloc(len_src + 1, sizeof(char));
	if (dest == NULL) {
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

int str_ccmp(const char *s1, const char *s2) {
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (toupper(*p1) == toupper(*p2)) {
		if (*p1 == '\0') {
			return 0;
		}
		++p1;
		++p2;
	}
	return toupper(*p2) > toupper(*p1) ? -1 : 1;
}
