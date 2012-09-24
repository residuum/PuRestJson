/*
 * [urlparams] encodes data as JSON and outputs it as a symbol.
 * */

#include "purest_json.h"

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

void urlparams_free_memory(t_urlparams *x) {
	t_key_value_pair *data_to_free;
	t_key_value_pair *next_data;

	data_to_free = x->first_data;
	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		freebytes(data_to_free->key, MAXPDSTRING);
		freebytes(data_to_free->value, MAXPDSTRING);
		freebytes(data_to_free, sizeof(t_key_value_pair));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->first_data = NULL;
	x->last_data = NULL;
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

	x->data_count = 0;
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

void urlparams_free (t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	urlparams_free_memory(x);
}

void urlparams_bang(t_urlparams *x) {
	int i;
	t_key_value_pair *data_member;
	char output[MAXPDSTRING];
	char *encoded_string = NULL;

	memset(output, 0x00, MAXPDSTRING);

	if (x->data_count > 0) {
		data_member = x->first_data;
		for (i = 0; i < x->data_count; i++) {
			strcat(output, data_member->key);
			strcat(output, "=");
			encoded_string = urlencode(data_member->value);
			strcat(output, encoded_string);
			if (encoded_string) {
				free(encoded_string);
			}
			if (i <x->data_count - 1) {
				strcat(output, "&");
			}
			data_member = data_member->next;
		}
		outlet_symbol(x->x_ob.ob_outlet, gensym(output));
	}
}

void urlparams_add(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *key;
	char *value;
	char temp_value[MAXPDSTRING];
	t_key_value_pair *created_data = NULL;
	int i;

	(void) selector;

	if (argcount < 2) {
		error("For method 'add' You need to specify a value.");
	} else {
		created_data = (t_key_value_pair *)getbytes(sizeof(t_key_value_pair));
		key = (char *)getbytes(MAXPDSTRING * sizeof(char));
		value = (char *)getbytes(MAXPDSTRING * sizeof(char));
		if (created_data == NULL || key == NULL || value == NULL) {
			error("Could not allocate memory.");
			return;
		}
		atom_string(argvec, key, MAXPDSTRING);
		created_data->key = key;
		atom_string(argvec + 1, value, MAXPDSTRING);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
		created_data->value = value;
		created_data->next = NULL;
		created_data->is_array = 0;
		if (x->first_data == NULL) {
			x->first_data = created_data;
		} else {
			x->last_data->next = created_data;
		}
		x->last_data = created_data;

		x->data_count++;
	}
}

void urlparams_clear(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	urlparams_free_memory(x);
}
