static char *string_create(size_t *newl, size_t strl) {
	char *gen;
	(*newl) = 1 + strl;
	gen = getbytes((*newl) * sizeof(char));
	if (gen == NULL) {
		MYERROR("not enough memory");
	}
	return memset(gen, 0x00, (*newl));
}

static void string_free(char *string, size_t *strl) {
	if ((*strl)) {
		freebytes(string, (*strl) * sizeof(char));
		(*strl) = 0;
		string = NULL;
	}
}

#ifndef NO_BACKSLASHES
static char *string_remove_backslashes(char *source_string, size_t *memsize) {
	char *cleaned_string = NULL;
	char *masking = "\\";
	char *segment;
	size_t len_src = strlen(source_string);

	cleaned_string = string_create(memsize, len_src);
	if (cleaned_string == NULL) {
		MYERROR("Unable to allocate memory\n");
	} else if (len_src > 0) {
		segment = strtok(source_string, masking);
		strcpy(cleaned_string, segment);
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
