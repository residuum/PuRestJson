#include "purest_json.h"

void output_json(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
	enum json_type outer_type = json_object_get_type(jobj);
	enum json_type inner_type;
	t_atom out_data[2];
	t_float out_float;
	char *remainder;
	float float_value;
	const char *string_value;
	int array_len;
	int i;

	if (is_error(jobj)) {
		error("Not a JSON object.");
	} else {
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
}

char *remove_backslashes(char *source_string) {
	char *dest = NULL;
	char remove[2] = "\\,";
	size_t len_src = strlen(source_string);
	int found;
	unsigned int i = 0;
	int j = 0;

	dest = (char *) calloc(len_src + 1, sizeof(char));
	if (dest == NULL) {
		printf("Unable to allocate memory\n");
	}
	memset(dest, 0x00,sizeof(char) * len_src + 1 );

	for ( i = 0; i < len_src; i++ ) {
		found = FALSE;
		if (source_string[i] == remove[0] && source_string[i +1] == remove[1]) {
			i++;
			found = TRUE;
		}

		if (FALSE == found)	{
			dest[j] = source_string[i];
		} else {
			dest[j] = ',';
		}
			j++;
	}
	return (dest);
}

int str_ccmp(const char *s1, const char *s2) {
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
