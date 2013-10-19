/*
 * [json-decode] decodes JSON data and outputs it as lists.
 * */

#include "json-decode.h"

#include "string.c"

static t_class *json_decode_class;

struct _json_decode {
	t_object x_ob;
	t_outlet *done_outlet;
};

static void json_dec_output_object(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
	t_atom out_data[2];
	enum json_type inner_type;

	json_object_object_foreach(jobj, key, val) { /* Passing through every json object */
		SETSYMBOL(&out_data[0], gensym(key));
		/* Problem with null as value */
		if (val == NULL) {
			SETSYMBOL(&out_data[1], gensym(""));
		} else {
			inner_type = json_object_get_type(val);
			switch (inner_type) {
				case json_type_boolean:
					SETFLOAT(&out_data[1], json_object_get_boolean(val) ? 1: 0);
					break;
				case json_type_double:
					SETFLOAT(&out_data[1], json_object_get_double(val));
					break;
				case json_type_int:
					SETFLOAT(&out_data[1], json_object_get_int(val));
					break;
				case json_type_string: 
					SETSYMBOL(&out_data[1], gensym(json_object_get_string(val)));
					break;
				case json_type_object:
					SETSYMBOL(&out_data[1], gensym(json_object_get_string(val)));
					json_object_put(val);
					break;
				case json_type_array:
					SETSYMBOL(&out_data[1], gensym(json_object_get_string(val)));
					break;
				case json_type_null:
					SETSYMBOL(&out_data[1], gensym(""));
					break;
			}
		}
		outlet_list(data_outlet, &s_list, 2, &out_data[0]);
	}
	outlet_bang(done_outlet);
}

static void json_dec_output_array(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
	int array_len;
	int i;

	array_len = json_object_array_length(jobj);
	for (i = 0; i < array_len; i++) {
		json_object *array_member = json_object_array_get_idx(jobj, i);
		if (!is_error(array_member)) {
			json_dec_output(array_member, data_outlet, done_outlet);
			/*json_object_put(array_member);*/
		}
	}
}

static void json_dec_output(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
	enum json_type outer_type;
	t_atom out_data[2];
	t_float out_float;

	outer_type = json_object_get_type(jobj);
	switch (outer_type) {
		case json_type_boolean:
			SETFLOAT(&out_data[0], json_object_get_boolean(jobj) ? 1: 0);
			out_float = atom_getfloat(&out_data[0]);
			outlet_float(data_outlet, out_float);
			outlet_bang(done_outlet);
			break;
		case json_type_double:
			SETFLOAT(&out_data[0], json_object_get_double(jobj));
			out_float = atom_getfloat(&out_data[0]);
			outlet_float(data_outlet, out_float);
			outlet_bang(done_outlet);
			break;
		case json_type_int:
			SETFLOAT(&out_data[0], json_object_get_int(jobj));
			out_float = atom_getfloat(&out_data[0]);
			outlet_float(data_outlet, out_float);
			outlet_bang(done_outlet);
			break;
		case json_type_string:
			outlet_symbol(data_outlet, gensym(json_object_get_string(jobj)));
			outlet_bang(done_outlet);
			break;
		case json_type_null:
			outlet_symbol(data_outlet, gensym(""));
			outlet_bang(done_outlet);
			break;
		case json_type_object:
			json_dec_output_object(jobj, data_outlet, done_outlet);
			break;
		case json_type_array: 
			json_dec_output_array(jobj, data_outlet, done_outlet);
			break;
	}
}

static void json_dec_output_string(char *json_string, t_json_decode *jdec) {
	json_object *jobj;

	jobj = json_tokener_parse(json_string);
	if (!is_error(jobj)) {
		json_dec_output(jobj, jdec->x_ob.ob_outlet, jdec->done_outlet);
		/* TODO: This sometimes results in a segfault. Why? */
		/*json_object_put(jobj);*/
	} else {
		pd_error(jdec, "Not a JSON object");
	}
}

void setup_json0x2ddecode(void) {
	json_decode_class = class_new(gensym("json-decode"), (t_newmethod)json_decode_new,
			0, sizeof(t_json_decode), 0, A_GIMME, 0);
	class_addsymbol(json_decode_class, (t_method)json_decode_string);
	class_addanything(json_decode_class, (t_method)json_decode_list);
	class_sethelpsymbol(json_decode_class, gensym("json"));
}

void *json_decode_new(t_symbol *sel, int argc, t_atom *argv) {
	t_json_decode *jdec = (t_json_decode*)pd_new(json_decode_class);

	(void) sel;
	(void) argc;
	(void) argv;

	outlet_new(&jdec->x_ob, NULL);
	jdec->done_outlet = outlet_new(&jdec->x_ob, &s_bang);
	return (void *)jdec;
}

void json_decode_string(t_json_decode *jdec, t_symbol *data) {
	size_t memsize = 0;
	char *original_string = data->s_name;
	char *json_string;

	if (original_string && strlen(original_string)) {
		json_string = string_remove_backslashes(original_string, &memsize);
		if (json_string != NULL) {
			json_dec_output_string(json_string, jdec);
			string_free(json_string, &memsize);
		}
	}
}

void json_decode_list(t_json_decode *jdec, t_symbol *sel, int argc, t_atom *argv) {
	size_t original_len = 1;
	char *original;
	size_t json_len = 0;
	char *json_string;
	char value[MAXPDSTRING];
	int i;
	int use_sel = (strcmp(sel->s_name, "symbol") && strcmp(sel->s_name, "list"));

	if (use_sel) {
		original_len += strlen(sel->s_name);
	}
	if (argc > 0) {
		for (i = 0; i < argc; i++) {
			atom_string(argv + i, value, MAXPDSTRING);
			original_len += 1 + strlen(value);
		}
	}
	original = getbytes(original_len * sizeof(char));

	if (!original) {
		return;
	}

	if (use_sel) {
		strcpy(original, sel->s_name);
	} else {
		memset(original, 0x00, MAXPDSTRING);
	}

	if (argc > 0) {
		for (i = 0; i < argc; i++) {
			atom_string(argv + i, value, MAXPDSTRING);
			if (strlen(original)) {
				strcat(original, " ");
			}
			strcat(original, value);
		}
	}
	if (strlen(original)) {
		json_string = string_remove_backslashes(original, &json_len);
		if (json_string != NULL) {
			json_dec_output_string(json_string, jdec);
			string_free(json_string, &json_len);
		}
	}
	string_free(original, &original_len);
}
