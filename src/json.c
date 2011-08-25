void setup_json_encoder(void) {
	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
		0, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
}

void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_encode *x = (t_json_encode*)pd_new(json_encode_class);
	x->data_count = 0;
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

void json_encode_bang(t_json_encode *x) {
	int i = 0; 
	char json_data[2 + MAX_ARRAY_SIZE * (2 * MAX_STRING_SIZE + 6)] = "{";
	if (x->data_count > 0) {
		for (i = 0; i < x->data_count; i++) {
			strcat(json_data, "\"");
			strcat(json_data, x->data[i * 2]);
			strcat(json_data, "\":");
			if(x->data[i * 2 +1][0] != '{' && x->data[i * 2 +1][0] != '[') {
				strcat(json_data, "\"");
			}
			strcat(json_data, x->data[i * 2 + 1]);
			if(x->data[i * 2 + 1][strlen(x->data[i * 2 + 1]) - 1] != '}' 
					&& x->data[i * 2 + 1][strlen(x->data[i * 2 + 1]) - 1] != ']') {
				strcat(json_data, "\"");
			}
			if (i < x->data_count - 1) {
				strcat(json_data, ",");
			}
		}
		strcat(json_data, "}");
		outlet_symbol(x->x_ob.ob_outlet, gensym(json_data));
	}
}

void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char key[MAX_STRING_SIZE];
	char value[MAX_STRING_SIZE];
	int i;
	if (argcount < 2) {
		error("For method 'add' You need to specify a value.");
	} else {
		atom_string(argvec, key, MAX_STRING_SIZE);
		strcpy(x->data[x->data_count * 2], key);
		atom_string(argvec + 1, value, MAX_STRING_SIZE);
		strcpy(x->data[x->data_count * 2 + 1], value);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, value, MAX_STRING_SIZE);
			strcat(x->data[x->data_count * 2 + 1], " ");
			strcat(x->data[x->data_count * 2 + 1], value);
		}
		x->data_count += 1;
	}
}

void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	x->data_count = 0;
}

void output_json(json_object *jobj, t_outlet *outlet) {
	char output_string[MAX_STRING_SIZE * MAX_ARRAY_SIZE] = "";
	enum json_type type;
	char val_string[MAX_STRING_SIZE];
	json_object_object_foreach(jobj, key, val) { /*Passing through every array element*/
		type = json_object_get_type(val);
		switch (type) {
			case json_type_boolean:
				strcat(output_string, key);
				strcat(output_string, " ");
				strcat(output_string, json_object_get_boolean(val) ? "1" : "0");
				strcat(output_string, " ");
				break;
			case json_type_double:
				strcat(output_string, key);
				strcat(output_string, " ");
				sprintf(val_string, "%f", json_object_get_double(val));
				strcat(output_string, val_string);
				strcat(output_string, " ");
				break;
			case json_type_int:
				strcat(output_string, key);
				strcat(output_string, " ");
				sprintf(val_string, "%d", json_object_get_int(val));
				strcat(output_string, val_string);
				strcat(output_string, " ");
				break;
			case json_type_string: 
				strcat(output_string, key);
				strcat(output_string, " ");
				strcat(output_string, json_object_get_string(val));
				strcat(output_string, " ");
				break;
			case json_type_object:
				error("TODO: What shall we do with a nested object?");
				break;
			case json_type_array:
				error("TODO: What shall we do with an array?");
				break;
			case json_type_null:
				error("TODO: What shall we do with a null object?");
				break;
		}
	}
	if (strlen(output_string) > 0) {
		post("%s", output_string);
		outlet_symbol(outlet, gensym(output_string));
	}
}
