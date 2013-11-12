/*
 * [urlparams] encodes data as JSON and outputs it as a symbol.
 * */

#include "urlparams.h"

#include "string.c"
#include "kvp.c"

static t_class *urlparams_class;

struct _urlparams {
	struct _kvp_store storage;
};

/* from http://www.geekhideout.com/urlcode.shtml */
static char urlp_tohex(char code) {
	static char hex[] = "0123456789abcdef";

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

void *urlparams_new(t_symbol *sel, int argc, t_atom *argv) {
	t_urlparams *urlp = (t_urlparams *)pd_new(urlparams_class);

	(void) sel;
	(void) argc;
	(void) argv;

	urlp->storage.data_count = 0;
	outlet_new(&urlp->storage.x_ob, NULL);
	return (void *)urlp;
}

void urlparams_free (t_urlparams *urlp, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)urlp);
}

void urlparams_bang(t_urlparams *urlp) {
	size_t i;
	struct _kvp *data_member;
	size_t output_len = 0;
	char *output;
	size_t encoded_len;
	char *encoded_string = NULL;

	if (urlp->storage.data_count == 0) {
		return;
	}

	data_member = urlp->storage.first_data;
	for (i = 0; i < urlp->storage.data_count; i++) {
		encoded_string = urlp_encode(data_member->value, &encoded_len);
		output_len += data_member->key_len + encoded_len + 2;
		string_free(encoded_string, &encoded_len);
		data_member = data_member->next;
	}
	output = getbytes(output_len * sizeof(char));

	data_member = urlp->storage.first_data;
	for (i = 0; i < urlp->storage.data_count; i++) {
		strcat(output, data_member->key);
		strcat(output, "=");
		encoded_string = urlp_encode(data_member->value, &encoded_len);
		strcat(output, encoded_string);
		if (encoded_string) {
			string_free(encoded_string, &encoded_len);
		}
		if (i < urlp->storage.data_count - 1) {
			strcat(output, "&");
		}
		data_member = data_member->next;
	}

	outlet_symbol(urlp->storage.x_ob.ob_outlet, gensym(output));
	string_free(output, &output_len);
}

void urlparams_add(t_urlparams *urlp, t_symbol *sel, int argc, t_atom *argv) {
	char key[MAXPDSTRING];
	size_t value_len = 0;
	char *value;
	char temp_value[MAXPDSTRING];
	int i;

	(void) sel;

	if (argc < 2) {
		pd_error(urlp, "For method 'add' You need to specify a value.");
		return;
	}

	atom_string(argv, key, MAXPDSTRING);

	for (i = 1; i < argc; i++) {
		atom_string(argv + i, temp_value, MAXPDSTRING);
		value_len += strlen(temp_value) + 1;
	}
	value = getbytes(value_len * sizeof(char));
	atom_string(argv + 1, value, MAXPDSTRING);
	for(i = 2; i < argc; i++) {
		atom_string(argv + i, temp_value, MAXPDSTRING);
		strcat(value, " ");
		strcat(value, temp_value);
	}
	kvp_add((struct _kvp_store *)urlp, key, value, 0);
	string_free(value, &value_len);
}

void urlparams_clear(t_urlparams *urlp, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)urlp);
}
