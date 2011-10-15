void setup_json_encoder(void) {
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
						json_object_array_add(value, create_object(x->data[j].value));
						array_member_numbers[array_member_count] = j;
						array_member_count++;
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
	x->data_count = 0;
}

void output_json(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
	/* NULL only in json-c after 2010-12-08, see 
https://github.com/json-c/json-c/commit/a503ee8217a9912f3c58acae33cf3d1d840dab6c */
	if (jobj == NULL || ((int)jobj < 0 && (int)jobj > -15)) {
		error("Not a JSON object: %d", jobj);
		return;
	}
	enum json_type outer_type = json_object_get_type(jobj);
	enum json_type inner_type;
	t_atom out_data[2];
	t_float out_float;
	char *remainder;
	float float_value;
	const char *string_value;
	int array_len;
	int i;

	switch (outer_type) {
		/* We really have a JSON object */
		case json_type_boolean:
			SETFLOAT(&out_data[1], json_object_get_boolean(jobj) ? 1: 0);
			out_float = atom_getfloat(&out_data[1]);
			outlet_float(data_outlet, out_float);
			outlet_bang(done_outlet);
			break;
		case json_type_double:
			SETFLOAT(&out_data[1], json_object_get_double(jobj));
			out_float = atom_getfloat(&out_data[1]);
			outlet_float(data_outlet, out_float);
			outlet_bang(done_outlet);
			break;
		case json_type_int:
			SETFLOAT(&out_data[1], json_object_get_int(jobj));
			out_float = atom_getfloat(&out_data[1]);
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
				output_json(array_member, data_outlet, done_outlet);
			}
			break;
	}
}

void setup_json_decoder(void) {
	json_decode_class = class_new(gensym("json-decode"), (t_newmethod)json_decode_new,
			0, sizeof(t_json_decode), 0, A_GIMME, 0);
	class_addsymbol(json_decode_class, (t_method)json_decode_string);
	class_sethelpsymbol(json_decode_class, gensym("json"));
}

void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_decode *x = (t_json_decode*)pd_new(json_decode_class);
	outlet_new(&x->x_ob, NULL);
	x->done_outlet = outlet_new(&x->x_ob, &s_bang);
	return (void *)x;
}

void json_decode_string(t_json_decode *x, t_symbol *data) {
	char *json_string = data->s_name;
	json_object *jobj;
	jobj = json_tokener_parse(json_string);
	output_json(jobj, x->x_ob.ob_outlet, x->done_outlet);
	json_object_put(jobj);
}
