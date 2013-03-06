static char *remove_backslashes(char *source_string, size_t *memsize) {
	char *cleaned_string = NULL;
	char *masking = "\\";
	char *segment;
	size_t len_src = strlen(source_string);

	(*memsize) = len_src + 1;

	cleaned_string = getbytes((*memsize) * sizeof(char));
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
