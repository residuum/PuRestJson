void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_encode *x = (t_json_encode*)pd_new(json_encode_class);
	x->data_count = 0;
	char data_array[2][MAX_ARRAY_SIZE][MAX_ARRAY_SIZE];
	x->data = (char ***)data_array;
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

void json_encode_bang(t_json_encode *x) {
	int i = 0; 
	char json_data[2 + MAX_ARRAY_SIZE * (2 * MAX_ARRAY_SIZE + 6)] = "{";
	if (x->data_count > 0) {
		for (i = 0; i < x->data_count; i++) {
			strcat(json_data, "\"");
			strcat(json_data, x->data + i * 2);
			strcat(json_data, "\":\"");
			strcat(json_data, x->data + i * 2 + 1);
			strcat(json_data, "\"");
			if (i < x->data_count - 1) {
				strcat(json_data, ",");
			}
		}
		strcat(json_data, "}");
		outlet_symbol(x->x_ob.ob_outlet, gensym(json_data));
		post("%s", json_data);
	}
}

void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char key[MAX_ARRAY_SIZE];
	char value[MAX_ARRAY_SIZE];
	if (argcount != 2) {
		error("In addition to 'add' You need to specify exactly 2 arguments: name of property and its value.");
	} else {
		atom_string(argvec, key, MAX_ARRAY_SIZE); 
		atom_string(argvec + 1, value, MAX_ARRAY_SIZE);
		post("key: %s, value: %s", key, value); 
		strcpy(x->data + x->data_count * 2 , key);
		strcpy(x->data + x->data_count * 2 + 1, value);
		x->data_count += 1;
	}
}

void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	x->data_count = 0;
}
