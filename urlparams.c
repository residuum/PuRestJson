/*
 * [urlparams] encodes data as JSON and outputs it as a symbol.
 * */

#include "purest_json.h"

#include "key_value_pair.c"

static t_class *urlparams_class;

struct _urlparams {
	struct _kvp_storage storage;
};

/* from http://www.geekhideout.com/urlcode.shtml */
static char to_hex(char code) {
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

/* from http://www.geekhideout.com/urlcode.shtml */
static char *urlencode(char *str, size_t *str_len) {
	char *pstr = str;
	char *buf;
	char *pbuf;

	(*str_len) = strlen(str) * 3 + 1;
	buf = (char *)getbytes((*str_len) * sizeof(char));
	pbuf = buf;
	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
			*pbuf++ = *pstr;
		} else {
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
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
	t_urlparams *x = (t_urlparams *)pd_new(urlparams_class);

	(void) sel;
	(void) argc;
	(void) argv;

	x->storage.data_count = 0;
	outlet_new(&x->storage.x_ob, NULL);
	return (void *)x;
}

void urlparams_free (t_urlparams *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_storage_free_memory((struct _kvp_storage *)x);
}

void urlparams_bang(t_urlparams *x) {
	int i;
	struct _key_value_pair *data_member;
	size_t output_len = 0;
	char *output;
	size_t encoded_len;
	char *encoded_string = NULL;

	if (x->storage.data_count > 0) {
		data_member = x->storage.first_data;
		for (i=0; i < x->storage.data_count; i++) {
			encoded_string = urlencode(data_member->value, &encoded_len);
			output_len += data_member->key_len + encoded_len + 2;
			freebytes(encoded_string, encoded_len * sizeof(char));
			data_member = data_member->next;
		}
		output = (char *)getbytes(output_len * sizeof(char));

		data_member = x->storage.first_data;
		for (i = 0; i < x->storage.data_count; i++) {
			strcat(output, data_member->key);
			strcat(output, "=");
			encoded_string = urlencode(data_member->value, &encoded_len);
			strcat(output, encoded_string);
			if (encoded_string) {
				freebytes(encoded_string, encoded_len * sizeof(char));
			}
			if (i < x->storage.data_count - 1) {
				strcat(output, "&");
			}
			data_member = data_member->next;
		}
		outlet_symbol(x->storage.x_ob.ob_outlet, gensym(output));
		freebytes(output, output_len * sizeof(char));
	}
}

void urlparams_add(t_urlparams *x, t_symbol *sel, int argc, t_atom *argv) {
	char key[MAXPDSTRING];
	size_t value_len = 0;
	char *value;
	char temp_value[MAXPDSTRING];
	struct _key_value_pair *created_data = NULL;
	int i;

	(void) sel;

	if (argc < 2) {
		error("For method 'add' You need to specify a value.");
	} else {
		atom_string(argv, key, MAXPDSTRING);

		for (i = 1; i < argc; i++) {
			atom_string(argv + i, temp_value, MAXPDSTRING);
			value_len += strlen(temp_value) + 1;
		}
		value = (char *)getbytes(value_len * sizeof(char));
		atom_string(argv + 1, value, MAXPDSTRING);
		for(i = 2; i < argc; i++) {
			atom_string(argv + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
		created_data = create_key_value_pair(key, value, 0);
		kvp_storage_add((struct _kvp_storage *)x, created_data);
		freebytes(value, value_len * sizeof(char));
	}
}

void urlparams_clear(t_urlparams *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_storage_free_memory((struct _kvp_storage *)x);
}
