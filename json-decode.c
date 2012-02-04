#include "purest_json.h"

static t_class *json_decode_class;

void setup_json0x2ddecode(void) {
	json_decode_class = class_new(gensym("json-decode"), (t_newmethod)json_decode_new,
			0, sizeof(t_json_decode), 0, A_GIMME, 0);
	class_addsymbol(json_decode_class, (t_method)json_decode_string);
	class_addlist(json_decode_class, (t_method)json_decode_list);
	class_sethelpsymbol(json_decode_class, gensym("json"));
}

void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_decode *x = (t_json_decode*)pd_new(json_decode_class);
	outlet_new(&x->x_ob, NULL);
	x->done_outlet = outlet_new(&x->x_ob, &s_bang);
	return (void *)x;
}

void json_decode_string(t_json_decode *x, t_symbol *data) {
	output_json_string(data->s_name, x->x_ob.ob_outlet, x->done_outlet);
}

void json_decode_list(t_json_decode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char json_string[MAX_STRING_SIZE];
	char value[MAX_STRING_SIZE];
	int i;
	if (argcount > 1) {
		atom_string(argvec + 1, json_string, MAX_STRING_SIZE);
		for (i = 2; i < argcount; i++) {
			atom_string(argvec + i, value, MAX_STRING_SIZE);
			strcat(json_string, value);
		}
		output_json_string(json_string, x->x_ob.ob_outlet, x->done_outlet);
	}
}
