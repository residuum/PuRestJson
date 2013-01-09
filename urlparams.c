/*
 * [urlparams] encodes data as JSON and outputs it as a symbol.
 * */

#include "purest_json.h"
#include "key_value_pair.c"

static t_class *urlparams_class;

/* from http://www.geekhideout.com/urlcode.shtml */
static char to_hex(char code) {
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

/* from http://www.geekhideout.com/urlcode.shtml */
static char *urlencode(char *str) {
	char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
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

void *urlparams_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_urlparams *x = (t_urlparams *)pd_new(urlparams_class);

	(void) selector;
	(void) argcount;
	(void) argvec;

	x->storage.data_count = 0;
	outlet_new(&x->storage.x_ob, NULL);
	return (void *)x;
}

void urlparams_free (t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	kvp_storage_free_memory((t_kvp_storage *)x);
}

void urlparams_bang(t_urlparams *x) {
	int i;
	t_key_value_pair *data_member;
	char output[MAXPDSTRING];
	char *encoded_string = NULL;

	memset(output, 0x00, MAXPDSTRING);

	if (x->storage.data_count > 0) {
		data_member = x->storage.first_data;
		for (i = 0; i < x->storage.data_count; i++) {
			strcat(output, data_member->key);
			strcat(output, "=");
			encoded_string = urlencode(data_member->value);
			strcat(output, encoded_string);
			if (encoded_string) {
				free(encoded_string);
			}
			if (i < x->storage.data_count - 1) {
				strcat(output, "&");
			}
			data_member = data_member->next;
		}
		outlet_symbol(x->storage.x_ob.ob_outlet, gensym(output));
	}
}

void urlparams_add(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char key[MAXPDSTRING];
	char value[MAXPDSTRING];
	char temp_value[MAXPDSTRING];
	t_key_value_pair *created_data = NULL;
	int i;

	(void) selector;
	
	if (argcount < 2) {
		error("For method 'add' You need to specify a value.");
	} else {
		atom_string(argvec, key, MAXPDSTRING);
		atom_string(argvec + 1, value, MAXPDSTRING);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
		created_data = create_key_value_pair(key, value, 0);
		if (x->storage.first_data == NULL) {
				x->storage.first_data = created_data;
			} else {
				x->storage.last_data->next = created_data;
			}
		x->storage.last_data = created_data;

		x->storage.data_count++;
	}
}

void urlparams_clear(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	kvp_storage_free_memory((t_kvp_storage *)x);
}
