#include "purest_json.h"

static t_class *json_encode_class;

void setup_json0x2dencode(void) {
	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
			0, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_array_add, gensym("array"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
	class_sethelpsymbol(json_encode_class, gensym("json"));
}

void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_encode *x = (t_json_encode*)pd_new(json_encode_class);

	(void) selector;
	(void) argcount;
	(void) argvec;

	x->data_count = 0;
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

void json_encode_bang(t_json_encode *x) {
	int i, j, k; 
	int array_member_numbers[x->data_count];
	int array_member_count = 0;
	short already_added = 0;
	json_object *jobj = json_object_new_object();
	json_object *value;
	json_object *array_members[x->data_count];

	if (x->data_count > 0) {
		for (i = 0; i < x->data_count; i++) {
			already_added = 0;
			/* Is it an array member? */
			if (x->data[i].is_array == 1) {
				value = json_object_new_array();
				for (j = i; j < x->data_count; j++) {
					if (strcmp(x->data[j].key, x->data[i].key) == 0) {
						for (k = 0; k < array_member_count; k++) {
							/* If already inserted, continue i loop */
							if (array_member_numbers[k] == j) {
								already_added = 1;
								break;
							}
						}
						if (already_added == 0) {
							json_object *array_member = create_object(x->data[j].value);
							json_object_array_add(value, array_member);
							array_member_numbers[array_member_count] = j;
							array_members[array_member_count] = array_member;
							array_member_count++;
						}
					}
				}
			} else {
				value = create_object(x->data[i].value);
			}
			if (already_added == 0) {
				json_object_object_add(jobj, x->data[i].key, value); 
			}
		}
		outlet_symbol(x->x_ob.ob_outlet, gensym(json_object_to_json_string(jobj)));
		for (i = 0; i < array_member_count; i++) {
			json_object_put(array_members[i]);
		}
		json_object_put(jobj);
	}
}

json_object *create_object(char *value) {
	json_object *object;
	char *parsed_string;
	/* if stored value is string is starting with { and ending with }, 
	   then create a json object from it. */
	if (value[0] == '{' && value[strlen(value) - 1] == '}') {
		parsed_string = remove_backslashes(value);
		object = json_tokener_parse(parsed_string);
		free(parsed_string);
	} else {
		object = json_object_new_string(value);
	}
	return object;
}

void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char key[MAX_STRING_SIZE];
	char value[MAX_STRING_SIZE];
	int i;

	(void) selector;

	if (argcount < 2) {
		error("For method 'add' You need to specify a value.");
	} else {
		atom_string(argvec, key, MAX_STRING_SIZE);
		strcpy(x->data[x->data_count].key, key);
		atom_string(argvec + 1, value, MAX_STRING_SIZE);
		strcpy(x->data[x->data_count].value, value);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, value, MAX_STRING_SIZE);
			strcat(x->data[x->data_count].value, " ");
			strcat(x->data[x->data_count].value, value);
		}
		x->data[x->data_count].is_array = 0;
		x->data_count++;
	}
}

void json_encode_array_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char key[MAX_STRING_SIZE];
	char value[MAX_STRING_SIZE];
	int i;

	(void) selector;

	if (argcount < 2) {
		error("For method 'array' You need to specify a value.");
	} else {
		atom_string(argvec, key, MAX_STRING_SIZE);
		strcpy(x->data[x->data_count].key, key);
		atom_string(argvec + 1, value, MAX_STRING_SIZE);
		strcpy(x->data[x->data_count].value, value);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, value, MAX_STRING_SIZE);
			strcat(x->data[x->data_count].value, " ");
			strcat(x->data[x->data_count].value, value);
		}
		x->data[x->data_count].is_array = 1;
		x->data_count++;
	}
}

void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	x->data_count = 0;
}
