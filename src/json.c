/**
 * Setup json_encoder
 * 
 * Performs setup of json_encoder object, initializes methods for inlet
 */
void setup_json_encoder(void) {
	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
		0, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
}

/**
 * Creates new instance of json_encoder
 */
void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_encode *x = (t_json_encode*)pd_new(json_encode_class);
	x->data_count = 0;
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

/**
 * Bang on json_encode outputs stored json data.
 */
void json_encode_bang(t_json_encode *x) {
	int i = 0; 
	json_object *jobj = json_object_new_object();
	json_object *value;
	if (x->data_count > 0) {
		for (i = 0; i < x->data_count; i++) {
			/* if stored value is string is starting with { and ending with }, 
			   then create a json object from it. */
			if (x->data[i * 2 + 1][0] == '{' && x->data[i * 2 + 1][strlen(x->data[i * 2 + 1]) -1] == '}') {
				post("%s", x->data[i * 2 + 1]);
				value = json_tokener_parse(remove_backslashes(x->data[i * 2 + 1]));
			} else {
				value = json_object_new_string(x->data[i * 2 + 1]);
			}
			json_object_object_add(jobj, x->data[i * 2], value); 
		}

		outlet_symbol(x->x_ob.ob_outlet, gensym(json_object_to_json_string(jobj)));
	}
}

/**
 * Add message on json_encode adds key/value pair to stored json data.
 */
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

/**
 * Clear message on json_encode clears stored json data.
 */
void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	x->data_count = 0;
}

/**
 * Outputs json data as string.
 *
 * @param jobj json object to output.
 * @param outlet outlet where data should be sent to.
 */
void output_json(json_object *jobj, t_outlet *outlet) {
	t_atom output_data[MAX_ARRAY_SIZE];
	size_t element_count = 0;
	enum json_type type;
	
	json_object_object_foreach(jobj, key, val) { /*Passing through every array element*/
		type = json_object_get_type(val);
		switch (type) {
			case json_type_boolean:
				SETSYMBOL(&output_data[element_count * 2], gensym(key));
				SETFLOAT(&output_data[element_count * 2 + 1], json_object_get_boolean(val) ? 1 : 0);
				element_count++;
				break;
			case json_type_double:
				SETSYMBOL(&output_data[element_count * 2], gensym(key));
				SETFLOAT(&output_data[element_count * 2 + 1], json_object_get_double(val));
				element_count++;
				break;
			case json_type_int:
				SETSYMBOL(&output_data[element_count * 2], gensym(key));
				SETFLOAT(&output_data[element_count * 2 + 1], json_object_get_int(val));
				element_count++;
				break;
			case json_type_string: 
				SETSYMBOL(&output_data[element_count * 2], gensym(key));
				SETSYMBOL(&output_data[element_count * 2 + 1], gensym(json_object_get_string(val)));
				element_count++;
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
	if (element_count > 1) {
		outlet_list(outlet, gensym(""), element_count * 2, output_data);
	}
}
/**
 * Setup json_decoder
 * 
 * Performs setup of json_decoder object, initializes methods for inlet.
 */
void setup_json_decoder(void) {
	json_decode_class = class_new(gensym("json-decode"), (t_newmethod)json_decode_new,
		0, sizeof(t_json_decode), 0, A_GIMME, 0);
	class_addsymbol(json_decode_class, (t_method)json_decode_string);
}

/**
 * Creates new instance of json_decoder.
 */
void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_decode *x = (t_json_decode*)pd_new(json_decode_class);
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

/**
 * Decodes string from inlet of json_decoder and outputs the json string as deserialized list.
 */
void json_decode_string(t_json_decode *x, t_symbol *data) {
	char *json_string = data->s_name;
	json_object *jobj;
	jobj = json_tokener_parse(json_string);
	output_json(jobj, x->x_ob.ob_outlet);
}
