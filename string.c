static char *string_create(size_t *newl, const size_t strl);
static void string_free(char *string, size_t *strl);
#ifndef NO_BACKSLASHES
static char *string_remove_backslashes(char *source_string, size_t *memsize);
#endif

/* begin implementations */
static char *string_create(size_t *const newl, const size_t strl) {
	char *gen;

	/* newl is not the length of the string, but the memory size */
	(*newl) = 1 + strl;
	gen = getbytes((*newl) * sizeof(char));
	if (gen == NULL) {
		MYERROR("not enough memory.");
		return gen;
	}
	return memset(gen, 0x00, (*newl));
}

static void string_free(char *string, size_t *const strl) {
	if ((*strl) > 0) {
		freebytes(string, (*strl) * sizeof(char));
		(*strl) = 0;
		string = NULL;
	}
}

#ifndef NO_BACKSLASHES
static char *string_remove_backslashes(char *const source_string, size_t *const memsize) {
	char *cleaned_string = NULL;
	const size_t len_src = strlen(source_string);

	cleaned_string = string_create(memsize, len_src);
	if (cleaned_string == NULL) {
		MYERROR("Unable to allocate memory.\n");
	} else if (len_src > 0) {
		char *masking = "\\";
		char *segment = strtok(source_string, masking);
		if (segment != NULL) {
			strcpy(cleaned_string, segment);
		}
		segment = strtok(NULL, masking);
		while (segment != NULL) {
			if (segment[0] != ',') {
				/* We keep the backslash */
				strcat(cleaned_string, masking);
			}
			strcat(cleaned_string, segment);
			segment = strtok(NULL, masking);
		}
	}
	return (cleaned_string);
}
#endif
