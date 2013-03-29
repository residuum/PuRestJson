/*
 * [json-decode] decodes JSON data and outputs it as lists.
 * */

#include "json-decode.h"

#include "string.c"

#ifndef JSON_C_MAJOR_VERSION
#define JSON_C_MAJOR_VERSION 0
#endif

#ifndef JSON_C_MINOR_VERSION
#define JSON_C_MINOR_VERSION 9
#endif

static t_class *json_decode_class;

struct _json_decode {
	t_object x_ob;
	t_outlet *done_outlet;
};

static int str_ccmp(const char *s1, const char *s2) {
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (toupper(*p1) == toupper(*p2)) {
		if (*p1 == '\0') {
			return 0;
		}
		++p1;
		++p2;
	}
	return toupper(*p2) > toupper(*p1) ? -1 : 1;
}

#if JSON_C_MAJOR_VERSION < 1 && JSON_C_MINOR_VERSION < 10
static char *lowercase_unicode(char *orig, size_t *memsize) {
	char *unicode_intro = "\\";
	char *segment; 
	char *cleaned_string;
	short i;
	short uni_len = 4; /*TODO: get real length, we just assume 4 for now */

	cleaned_string = string_create(memsize, strlen(orig));
	if (cleaned_string != NULL) {
		if (strlen(orig) > 0) {
			segment = strtok(orig, unicode_intro);
			memset(cleaned_string, 0x00, strlen(orig) + 1);
			strcpy(cleaned_string, segment);
			segment = strtok(NULL, unicode_intro);

			while(segment != NULL) {
				strcat(cleaned_string, unicode_intro);
				if (segment[0] == 'u') {
					for (i = 1; i < 1 + uni_len; i++) {
						switch (segment[i]) {
							case 'A':
								segment[i] = 'a';
								break;
							case 'B':
								segment[i] = 'b';
								break;
							case 'C':
								segment[i] = 'c';
								break;
							case 'D':
								segment[i] = 'd';
								break;
							case 'E':
								segment[i] = 'e';
								break;
							case 'F':
								segment[i] = 'f';
								break;
						}
					}
				}
				strcat(cleaned_string, segment);
				segment = strtok(NULL, unicode_intro);
			}

		}
	} else {
		MYERROR("Could not allocate memory");
	}
	return cleaned_string;
}
#endif

static void output_json(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
	enum json_type outer_type;
	enum json_type inner_type;
	t_atom out_data[2];
	t_float out_float;
	char *remainder;
	float float_value;
	const char *string_value;
	int array_len;
	int i;

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
			;
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
							/* Float values might come as string */
							string_value = json_object_get_string(val);
							float_value = (float)strtod(string_value, &remainder);
							/* String to float has no remainder => float */
							if (strlen(remainder) == 0) {
								SETFLOAT(&out_data[1], float_value);
								/* Boolean values might come as string */
							} else if (str_ccmp(string_value, "true") == 0) {
								SETFLOAT(&out_data[1], 1);
							} else if (str_ccmp(string_value, "false") == 0) {
								SETFLOAT(&out_data[1], 0);
								/* String */
							} else {
								SETSYMBOL(&out_data[1], gensym(string_value));
							}
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
			break;
		case json_type_array: 
			;
			array_len = json_object_array_length(jobj);
			for (i = 0; i < array_len; i++) {
				json_object *array_member = json_object_array_get_idx(jobj, i);
				if (!is_error(array_member)) {
					output_json(array_member, data_outlet, done_outlet);
					/*json_object_put(array_member);*/
				}
			}
			break;
	}
}

static void output_json_string(char *json_string, t_outlet *data_outlet, t_outlet *done_outlet) {
	json_object *jobj;
#if JSON_C_MAJOR_VERSION < 1 && JSON_C_MINOR_VERSION < 10
	size_t memsize = 0;
	/* Needed because of bug in json-c 0.9 */
	char* corrected_json_string = lowercase_unicode(json_string, &memsize);
	/* Parse JSON */
	jobj = json_tokener_parse(corrected_json_string);
#else
	jobj = json_tokener_parse(json_string);
#endif

	if (!is_error(jobj)) {
		output_json(jobj, data_outlet, done_outlet);
		/* TODO: This sometimes results in a segfault. Why? */
		/*json_object_put(jobj);*/
	} else {
		MYERROR("Not a JSON object");
	}
#if JSON_C_MAJOR_VERSION < 1 && JSON_C_MINOR_VERSION < 10
	if (corrected_json_string != NULL){
		string_free(corrected_json_string, &memsize);
	}
#endif
}

void setup_json0x2ddecode(void) {
	json_decode_class = class_new(gensym("json-decode"), (t_newmethod)json_decode_new,
			0, sizeof(t_json_decode), 0, A_GIMME, 0);
	class_addsymbol(json_decode_class, (t_method)json_decode_string);
	class_addanything(json_decode_class, (t_method)json_decode_list);
	class_sethelpsymbol(json_decode_class, gensym("json"));
}

void *json_decode_new(t_symbol *sel, int argc, t_atom *argv) {
	t_json_decode *x = (t_json_decode*)pd_new(json_decode_class);

	(void) sel;
	(void) argc;
	(void) argv;

	outlet_new(&x->x_ob, NULL);
	x->done_outlet = outlet_new(&x->x_ob, &s_bang);
	return (void *)x;
}

void json_decode_string(t_json_decode *x, t_symbol *data) {
	size_t memsize = 0;
	char *original_string = data->s_name;
	char *json_string;

	if (original_string && strlen(original_string)) {
		json_string = string_remove_backslashes(original_string, &memsize);
		if (json_string != NULL) {
			output_json_string(json_string, x->x_ob.ob_outlet, x->done_outlet);
			string_free(json_string, &memsize);
		}
	}
}

void json_decode_list(t_json_decode *x, t_symbol *sel, int argc, t_atom *argv) {
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

	if (original) {
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
				output_json_string(json_string, x->x_ob.ob_outlet, x->done_outlet);
				string_free(json_string, &json_len);
			}
		}
		string_free(original, &original_len);
	}
}
