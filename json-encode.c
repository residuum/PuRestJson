/*
 * [json-encodes] encodes data as JSON and outputs it as a symbol.
 * */

#include "json-encode.h"
#include <sys/stat.h>
#include <stdio.h>

#include "string.c"
#include "kvp.c"

static t_class *json_encode_class;

struct _json_encode {
	struct _kvp_store storage;
	t_canvas *x_canvas;
};

static json_object *create_object(char *value) {
	json_object *object;
	char *parsed_string;
	size_t memsize = 0;
	/* if stored value is string is starting with { and ending with }, 
	   then create a json object from it. */
	if (value[0] == '{' && value[strlen(value) - 1] == '}') {
		parsed_string = string_remove_backslashes(value, &memsize);
		object = json_tokener_parse(parsed_string);
		string_free(parsed_string, &memsize);
	} else {
		object = json_object_new_string(value);
	}
	return object;
}

static void load_json_data(t_json_encode *x, json_object *jobj) {
	enum json_type outer_type;
	enum json_type inner_type;
	char *value;
	size_t value_len = 0;
	int array_len;
	int i;

	kvp_store_free_memory((struct _kvp_store *)x);
	outer_type = json_object_get_type(jobj);

	switch (outer_type) {
		case json_type_object:
			;
			json_object_object_foreach(jobj, key, val) {
				inner_type = json_object_get_type(val);
				switch (inner_type) {
					case json_type_boolean:
						kvp_add((struct _kvp_store *)x, key, json_object_get_boolean(val) ? "1" : "0", 0);
						break;
					case json_type_double:
						value = string_create(&value_len, snprintf(NULL, 0, "%f", json_object_get_double(val)));
						sprintf(value, "%f", json_object_get_double(val));
						kvp_add((struct _kvp_store *)x, key, value, 0);
						string_free(value, &value_len);
						break;
					case json_type_int:
						value = string_create(&value_len, snprintf(NULL, 0, "%d", json_object_get_int(val)));
						sprintf(value, "%d", json_object_get_int(val));
						kvp_add((struct _kvp_store *)x, key, value, 0);
						string_free(value, &value_len);
						break;
					case json_type_string:
						value = string_create(&value_len, snprintf(NULL, 0, "%s", json_object_get_string(val)));
						sprintf(value, "%s", json_object_get_string(val));
						kvp_add((struct _kvp_store *)x, key, value, 0);
						string_free(value, &value_len);
						break;
					case json_type_object:
						value = string_create(&value_len, snprintf(NULL, 0, "%s", json_object_get_string(val)));
						sprintf(value, "%s", json_object_get_string(val));
						kvp_add((struct _kvp_store *)x, key, value, 0);
						string_free(value, &value_len);
						json_object_put(val);
						break;
					case json_type_array:
						array_len = json_object_array_length(val);
						for (i = 0; i < array_len; i++) {
							json_object *array_member = json_object_array_get_idx(val, i);
							if (!is_error(array_member)) {
								value = string_create(&value_len, 
										snprintf(NULL, 0, "%s", json_object_get_string(array_member)));
								sprintf(value, "%s", json_object_get_string(array_member));
								kvp_add((struct _kvp_store *)x, key, value, 1);
								string_free(value, &value_len);
							}
						}
						break;
					case json_type_null:
						kvp_add((struct _kvp_store *)x, key, "", 0);
						break;
				}
			}
			break;
		default: 
			pd_error(x, "This JSON data cannot be represented internally, sorry");
			break;
	}
}

static t_symbol *get_json_symbol(t_json_encode *x) {
	size_t i, j, k; 
	size_t array_member_numbers[x->storage.data_count];
	size_t array_member_count = 0;
	unsigned char already_added = 0;
	struct _kvp *data_member;
	struct _kvp *data_member_compare;
	json_object *jobj = json_object_new_object();
	json_object *value;
	json_object *array_members[x->storage.data_count];
	t_symbol *json_symbol = NULL;

	if (x->storage.data_count == 0) {
		json_symbol = gensym("");
		return json_symbol;
	}

	data_member = x->storage.first_data;
	for (i = 0; i < x->storage.data_count; i++) {
		already_added = 0;
		/* Is it an array member? */
		if (data_member->is_array == 1) {
			value = json_object_new_array();
			data_member_compare = data_member;
			for (j = i; j < x->storage.data_count; j++) {
				if (strcmp(data_member_compare->key, data_member->key) == 0) {
					for (k = 0; k < array_member_count; k++) {
						/* If already inserted, continue i loop */
						if (array_member_numbers[k] == j) {
							already_added = 1;
							break;
						}
					}
					if (already_added == 0) {
						json_object *array_member = create_object(data_member_compare->value);
						json_object_array_add(value, array_member);
						array_member_numbers[array_member_count] = j;
						array_members[array_member_count] = array_member;
						array_member_count++;
					}
				}
				data_member_compare = data_member_compare->next;
			}
		} else {
			value = create_object(data_member->value);
		}
		if (already_added == 0) {
			json_object_object_add(jobj, data_member->key, value); 
		}
		data_member = data_member->next;
	}
	json_symbol = gensym(json_object_to_json_string(jobj));
	for (i = 0; i < array_member_count; i++) {
		json_object_put(array_members[i]);
	}
	json_object_put(jobj);
	return json_symbol;
}

void setup_json0x2dencode(void) {
	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
			(t_method)json_encode_free, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("array"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_read, gensym("read"), A_SYMBOL, A_DEFSYM, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_write, gensym("write"), A_SYMBOL, A_DEFSYM, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
	class_sethelpsymbol(json_encode_class, gensym("json"));
}

void *json_encode_new(t_symbol *sel, int argc, t_atom *argv) {
	t_json_encode *x = (t_json_encode *)pd_new(json_encode_class);

	(void) sel;
	(void) argc;
	(void) argv;

	x->storage.data_count = 0;
	outlet_new(&x->storage.x_ob, NULL);
	x->x_canvas = canvas_getcurrent();
	return (void *)x;
}

void json_encode_free (t_json_encode *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)x);
}

void json_encode_bang(t_json_encode *x) {
	outlet_symbol(x->storage.x_ob.ob_outlet, get_json_symbol(x));
}

void json_encode_add(t_json_encode *x, t_symbol *sel, int argc, t_atom *argv) {
	char key[MAXPDSTRING];
	size_t value_len = 0;
	char *value;
	char temp_value[MAXPDSTRING];
	int i;
	unsigned char is_array = 0;

	if (sel == gensym("array")) {
		is_array = 1;
	}

	if (argc < 2) {
		pd_error(x, "For method '%s' You need to specify a value.", is_array ? "array": "add");
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
	kvp_add((struct _kvp_store *)x, key, value, is_array);
	string_free(value, &value_len);
}

void json_encode_read(t_json_encode *x, t_symbol *filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	struct stat st;
	char *json_string;
	json_object *jobj;

	canvas_makefilename(x->x_canvas, filename->s_name, buf, MAXPDSTRING);
	file = fopen(buf, "r");
	if (!file) {
		pd_error(x, "%s: read failed", filename->s_name);
		return;
	}
	
	stat(buf, &st);
	json_string = getbytes((st.st_size + 1) * sizeof(char));
	json_string[st.st_size] = 0x00;
	fread(json_string, sizeof(char), st.st_size, file);
	fclose(file);
	jobj = json_tokener_parse(json_string);
	freebytes(json_string, (st.st_size + 1) * sizeof(char));
	if (!is_error(jobj)) {
		load_json_data(x, jobj);
		json_object_put(jobj);
	} else {
		post("File does not contain valid JSON.");
	}
}

void json_encode_write(t_json_encode *x, t_symbol *filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	t_symbol *json_symbol = get_json_symbol(x);
	char *json_string = json_symbol->s_name;

	if (!json_string) {
		post("No JSON data for writing available.");
		return;
	}

	canvas_makefilename(x->x_canvas, filename->s_name, buf, MAXPDSTRING);
	if ((file = fopen(buf, "w"))) {
		fprintf(file, json_string);
		fclose(file);
	} else {
		pd_error(x, "%s: write failed", filename->s_name);
	}
}

void json_encode_clear(t_json_encode *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)x);
}
