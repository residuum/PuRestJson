/*
 * [json-encodes] encodes data as JSON and outputs it as a symbol.
 * */

#include "purest_json.h"
#include <sys/stat.h>
#include <stdio.h>

static t_key_value_pair *create_key_value_pair(char *key, char *value, int is_array){
	t_key_value_pair *created_data = NULL;

	created_data = (t_key_value_pair *)getbytes(sizeof(t_key_value_pair));
	created_data->key = (char *)getbytes(MAXPDSTRING * sizeof(char));
	created_data->value = (char *)getbytes(MAXPDSTRING * sizeof(char));
	if (created_data == NULL || key == NULL || value == NULL) {
		error("Could not get data");
		return NULL;
	}
	strcpy(created_data->key, key);
	strcpy(created_data->value, value);
	created_data->value = value;
	created_data->next = NULL;
	created_data->is_array = is_array;

	return created_data;
}

static t_class *json_encode_class;

static void json_encode_free_memory(t_json_encode *x) {
	t_key_value_pair *data_to_free;
	t_key_value_pair *next_data;

	data_to_free = x->first_data;
	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		/* TODO: Investigate the reason for segfault */
		/*freebytes(data_to_free->key, MAXPDSTRING);
		freebytes(data_to_free->value, MAXPDSTRING);*/
		freebytes(data_to_free, sizeof(t_key_value_pair));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->first_data = NULL;
	x->last_data = NULL;
}

static json_object *create_object(char *value) {
	json_object *object;
	char *parsed_string;
	size_t memsize = 0;
	/* if stored value is string is starting with { and ending with }, 
	   then create a json object from it. */
	if (value[0] == '{' && value[strlen(value) - 1] == '}') {
		parsed_string = remove_backslashes(value, memsize);;
		object = json_tokener_parse(parsed_string);
		freebytes(parsed_string, memsize);
	} else {
		object = json_object_new_string(value);
	}
	return object;
}

static void load_json_data(t_json_encode *x, json_object *jobj) {
	enum json_type outer_type;
	enum json_type inner_type;
	t_key_value_pair *new_pair;
	char value[MAXPDSTRING];

	json_encode_free_memory(x);
	outer_type = json_object_get_type(jobj);
	post("%i", outer_type);
	switch (outer_type) {
		case json_type_object:
			;
			json_object_object_foreach(jobj, key, val) {
				new_pair = NULL;
				inner_type = json_object_get_type(val);
				switch (inner_type) {
					case json_type_boolean:
						new_pair = create_key_value_pair(key, json_object_get_boolean(val) ? "1" : "0", 0);
						break;
					case json_type_double:
						sprintf(value, "%f", json_object_get_double(val));
						new_pair = create_key_value_pair(key, value, 0);
						break;
					case json_type_int:
						sprintf(value, "%i", json_object_get_int(val));
						new_pair = create_key_value_pair(key, value, 0);
						break;
					case json_type_string:
						sprintf(value,  "%s", json_object_get_string(val));
						new_pair = create_key_value_pair(key, value, 0);
						break;
					case json_type_object:
						sprintf(value, "%s", json_object_get_string(val));
						new_pair = create_key_value_pair(key, value, 0);
						json_object_put(val);
						break;
					case json_type_array:
						/* TODO: Split array */ 
						/*new_pair = create_key_value_pair(key, json_object_get_string(val), 0);*/
						break;
					case json_type_null:
						new_pair = create_key_value_pair(key, "", 0);
						break;
				}

				if (new_pair) {
					x->data_count++;
					if (!x->first_data) {
						x->first_data = new_pair;
					} else {
						x->last_data->next = new_pair;
					}
					x->last_data = new_pair;
				}
			}
			break;
		default: 
			error("This JSON data cannot be represented internally, sorry");
			break;
	}
}

static t_symbol *get_json_symbol(t_json_encode *x) {
	int i, j, k; 
	int array_member_numbers[x->data_count];
	int array_member_count = 0;
	short already_added = 0;
	t_key_value_pair *data_member;
	t_key_value_pair *data_member_compare;
	json_object *jobj = json_object_new_object();
	json_object *value;
	json_object *array_members[x->data_count];
	t_symbol *json_symbol = NULL;

	if (x->data_count > 0) {
		data_member = x->first_data;
		for (i = 0; i < x->data_count; i++) {
			already_added = 0;
			/* Is it an array member? */
			if (data_member->is_array == 1) {
				value = json_object_new_array();
				data_member_compare = data_member;
				for (j = i; j < x->data_count; j++) {
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
	}
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

void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_encode *x = (t_json_encode *)pd_new(json_encode_class);

	(void) selector;
	(void) argcount;
	(void) argvec;

	x->data_count = 0;
	outlet_new(&x->x_ob, NULL);
	x->x_canvas = canvas_getcurrent();
	return (void *)x;
}

void json_encode_free (t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	json_encode_free_memory(x);
}

void json_encode_bang(t_json_encode *x) {
	outlet_symbol(x->x_ob.ob_outlet, get_json_symbol(x));
}

void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char key[MAXPDSTRING];
	char value[MAXPDSTRING];
	char temp_value[MAXPDSTRING];
	t_key_value_pair *created_data = NULL;
	int i;
	int is_array = 0;

	if (selector == gensym("array")) {
		is_array = 1;
	}

	if (argcount < 2) {
		error("For method '%s' You need to specify a value.", is_array ? "array": "add");
	} else {
		atom_string(argvec, key, MAXPDSTRING);
		atom_string(argvec + 1, value, MAXPDSTRING);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
		created_data = create_key_value_pair(key, value, is_array);
		if (x->first_data == NULL) {
				x->first_data = created_data;
			} else {
				x->last_data->next = created_data;
			}
		x->last_data = created_data;

		x->data_count++;
	}
}

void json_encode_read(t_json_encode *x, t_symbol *filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	struct stat st;
	char *json_string;
	json_object *jobj;

	canvas_makefilename(x->x_canvas, filename->s_name, buf, MAXPDSTRING);
	if ((file = fopen(buf, "r"))) {
		stat(buf, &st);
		json_string = (char *)getbytes((st.st_size + 1) * sizeof(char));
		json_string[st.st_size] = 0x00;
		fread(json_string, sizeof(char), st.st_size, file);
		fclose(file);
		jobj = json_object_new_string(json_string);
		freebytes(json_string, (st.st_size + 1) * sizeof(char));
		if (!is_error(jobj)) {
			load_json_data(x, jobj);
			/*TODO*/
			json_object_put(jobj);
		} else {
			post("File does not contain valid JSON.");
		}


	} else {
		post("Cannot open file %s for reading", filename->s_name);
	}
}

void json_encode_write(t_json_encode *x, t_symbol *filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	t_symbol *json_symbol = get_json_symbol(x);
	char *json_string = json_symbol->s_name;

	if (json_string) {
		canvas_makefilename(x->x_canvas, filename->s_name, buf, MAXPDSTRING);
		if ((file = fopen(buf, "w"))) {
			fprintf(file, json_string);
			fclose(file);
		} else {
			post("Cannot open %s for writing", filename->s_name);
		}
	} else {
		post("No JSON data for writing available.");
	}
}

void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	json_encode_free_memory(x);
}
