/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2022 Thomas Mayer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

/* creates string */
static char *string_create(size_t *newl, const size_t strl);
/* frees string */
static void string_free(char *string, size_t *strl);
/* suppresses warning, nothing special */
#ifndef NO_BACKSLASHES
/* removes pd added backslashes from string */
static char *string_remove_backslashes(const char *source_string, size_t *memsize);
#endif

/* begin implementations */
static char *string_create(size_t *const newl, const size_t strl) {
	char *gen;

	/* newl is not the length of the string, but the memory size */
	(*newl) = 1 + strl;
	gen = getbytes((*newl) * sizeof(char));
	if (gen == NULL) {
		pd_error(0, "not enough memory.");
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

/* suppresses warning, nothing special */
#ifndef NO_BACKSLASHES
static char *string_remove_backslashes(const char *const _source_string, size_t *const memsize) {
	char *cleaned_string = NULL;
	char source_string_stack[MAXPDSTRING];
	const size_t len_src = strlen(_source_string);
	size_t sslen = 0;
	char*source_string = (len_src<MAXPDSTRING)?source_string_stack:string_create(&sslen, len_src);
	strcpy(source_string, _source_string);

	cleaned_string = string_create(memsize, len_src);
	if (cleaned_string == NULL) {
		pd_error(0, "Unable to allocate memory.\n");
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
	if(source_string != source_string_stack)
		string_free(source_string, &sslen);
	return (cleaned_string);
}
#endif
