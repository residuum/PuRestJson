/*
 * [json-decode] decodes JSON data and outputs it as lists.
 * */

#include "purest_json.h"

static t_class *json_decode_class;

void setup_json0x2ddecode(void) {
	json_decode_class = class_new(gensym("json-decode"), (t_newmethod)json_decode_new,
			0, sizeof(t_json_decode), 0, A_GIMME, 0);
	class_addsymbol(json_decode_class, (t_method)json_decode_string);
	/* Commented because it defeats json_decode_string: [l2s] is always needed */
	/*class_addlist(json_decode_class, (t_method)json_decode_list);*/
	class_sethelpsymbol(json_decode_class, gensym("json"));
}

void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_decode *x = (t_json_decode*)pd_new(json_decode_class);

	(void) selector;
	(void) argcount;
	(void) argvec;

	outlet_new(&x->x_ob, NULL);
	x->done_outlet = outlet_new(&x->x_ob, &s_bang);
	return (void *)x;
}

void json_decode_string(t_json_decode *x, t_symbol *data) {
	size_t memsize = 0;
	char *json_string = remove_backslashes(data->s_name, memsize);

	if (json_string != NULL) {
		output_json_string(json_string, x->x_ob.ob_outlet, x->done_outlet);
		freebytes(json_string, memsize);
	}
}

void json_decode_list(t_json_decode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char json_string[MAXPDSTRING];
	char value[MAXPDSTRING];
	int i;

	(void) selector;


	if (argcount > 1) {
		atom_string(argvec + 1, json_string, MAXPDSTRING);
		for (i = 2; i < argcount; i++) {
			atom_string(argvec + i, value, MAXPDSTRING);
			strcat(json_string, value);
		}
		output_json_string(json_string, x->x_ob.ob_outlet, x->done_outlet);
	}
}
