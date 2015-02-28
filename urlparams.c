/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2015 Thomas Mayer

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

/*
 * [urlparams] encodes data as JSON and outputs it as a symbol.
 * */

#include "urlparams.h"
#include <math.h>

#include "uthash/src/uthash.h"
#include "string.c"
#include "kvp.c"

static t_class *urlparams_class;

struct _urlparams {
	struct _kvp_store storage;
};

static char urlp_tohex(const char code);
static char *urlp_encode(char *str, size_t *str_len);

/* begin implementations */
/* from http://www.geekhideout.com/urlcode.shtml */
static char urlp_tohex(const char code) {
	static const char hex[] = "0123456789abcdef";

	return hex[code & 15];
}

/* from http://www.geekhideout.com/urlcode.shtml */
static char *urlp_encode(char *str, size_t *str_len) {
	char *pstr = str;
	char *buf;
	char *pbuf;

	(*str_len) = strlen(str) * 3 + 1;
	buf = getbytes((*str_len) * sizeof(char));
	pbuf = buf;
	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
			*pbuf++ = *pstr;
		} else {
			*pbuf++ = '%', *pbuf++ = urlp_tohex(*pstr >> 4), *pbuf++ = urlp_tohex(*pstr & 15);
		}
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

void urlparams_setup(void) {
	urlparams_class = class_new(gensym("urlparams"), (t_newmethod)urlparams_new,
			(t_method)urlparams_free, sizeof(t_urlparams), 0, A_GIMME, 0);
	class_addbang(urlparams_class, (t_method)urlparams_bang);
	class_addmethod(urlparams_class, (t_method)urlparams_add, gensym("add"), A_GIMME, 0);
	class_addmethod(urlparams_class, (t_method)urlparams_clear, gensym("clear"), A_GIMME, 0);
}

void *urlparams_new(const t_symbol *const sel, const int argc, const t_atom *argv) {
	t_urlparams *const urlp = (t_urlparams *)pd_new(urlparams_class);

	(void) sel;
	(void) argc;
	(void) argv;

	outlet_new(&urlp->storage.x_ob, NULL);
	purest_json_lib_info("urlparams");
	return (void *)urlp;
}

void urlparams_free (t_urlparams *const urlp, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)urlp);
}

void urlparams_bang(t_urlparams *const urlp) {
	struct _kvp *it;
	size_t output_len = 0;
	char *output;
	size_t encoded_key_len;
	char *encoded_key_string = NULL;
	size_t encoded_val_len;
	char *encoded_val_string = NULL;

	if (!HASH_COUNT(urlp->storage.data)) {
		outlet_symbol(urlp->storage.x_ob.ob_outlet, gensym(""));
		return;
	}

	for(it = urlp->storage.data; it != NULL; it = it->hh.next) {
		encoded_key_string = urlp_encode(it->key, &encoded_key_len);
		encoded_val_string = urlp_encode(it->value->val.s, &encoded_val_len);
		output_len += encoded_key_len + encoded_val_len + 2;
		string_free(encoded_key_string, &encoded_key_len);
		string_free(encoded_val_string, &encoded_val_len);
	}
	output = getbytes(output_len * sizeof(char));

	for(it = urlp->storage.data; it != NULL; it = it->hh.next) {
		encoded_key_string = urlp_encode(it->key, &encoded_key_len);
		encoded_val_string = urlp_encode(it->value->val.s, &encoded_val_len);
		strcat(output, encoded_key_string);
		strcat(output, "=");
		strcat(output, encoded_val_string);
		if (encoded_key_string) {
			string_free(encoded_key_string, &encoded_key_len);
		}
		if (encoded_val_string) {
			string_free(encoded_val_string, &encoded_val_len);
		}
		if (it->hh.next != NULL) {
			strcat(output, "&");
		}
	}

	outlet_symbol(urlp->storage.x_ob.ob_outlet, gensym(output));
	string_free(output, &output_len);
}

void urlparams_add(t_urlparams *const urlp, const t_symbol *const sel, const int argc, t_atom *const argv) {
	char key[MAXPDSTRING];
	size_t value_len = 0;
	char *value;
	char temp_value[MAXPDSTRING];

	(void) sel;

	if (argc < 2) {
		pd_error(urlp, "For method 'add' You need to specify a value.");
		return;
	}

	atom_string(argv, key, MAXPDSTRING);

	for (int i = 1; i < argc; i++) {
		atom_string(argv + i, temp_value, MAXPDSTRING);
		value_len += strlen(temp_value) + 1;
	}
	value = getbytes(value_len * sizeof(char));
	atom_string(argv + 1, value, MAXPDSTRING);
	for(int i = 2; i < argc; i++) {
		atom_string(argv + i, temp_value, MAXPDSTRING);
		strcat(value, " ");
		strcat(value, temp_value);
	}
	kvp_add((struct _kvp_store *)urlp, key, kvp_val_create(value, 0), 0);
	string_free(value, &value_len);
}

void urlparams_clear(t_urlparams *const urlp, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)urlp);
}
