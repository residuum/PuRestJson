#include "purest_json.h"

char *lowercase_unicode(char *orig, size_t memsize) {
	char *unicode_intro = "\\u";
	char *tmp = strstr(orig, unicode_intro);
	char *tmp_without_intro;
	char *return_string;
	short i;
	short uni_len = 4; /*TODO: get real length, we just assume 4 for now */

	memsize = (strlen(orig) + 1) * sizeof(char);
	return_string = (char *)getbytes(memsize);
	if (return_string != NULL) {
		if (tmp) {
			memset(return_string, 0x00, strlen(orig) + 1);
			strncpy(return_string, orig, strlen(orig) - strlen(tmp));
			do {	
				for (i = 2; i < 2 + uni_len; i++) {
					switch (tmp[i]) {
						case 'A':
							tmp[i] = 'a';
							break;
						case 'B':
							tmp[i] = 'b';
							break;
						case 'C':
							tmp[i] = 'c';
							break;
						case 'D':
							tmp[i] = 'd';
							break;
						case 'E':
							tmp[i] = 'e';
							break;
						case 'F':
							tmp[i] = 'f';
							break;
					}
				}
				strcat(return_string, unicode_intro);
				tmp_without_intro = tmp + 2;
				tmp = strstr(tmp_without_intro, unicode_intro);
				if (tmp) {
					strncat(return_string, tmp_without_intro, strlen(tmp_without_intro) - strlen(tmp));
				} else {
					strcat(return_string, tmp_without_intro);
				}
			} while(tmp);
		} else {
			strcpy(return_string, orig);
		}
	} else {
		error("Could not allocate memory");
	}
	return return_string;
}

void output_json(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet) {
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
					json_object_put(array_member);
				}
			}
			break;
	}
}

void output_json_string(char *json_string, t_outlet *data_outlet, t_outlet *done_outlet) {
	json_object *jobj;
	size_t memsize = 0;
	/* Needed because of bug in json-c 0.9 */
	char* corrected_json_string = lowercase_unicode(json_string, memsize);
	/* Parse JSON */
	jobj = json_tokener_parse(corrected_json_string);
	if (!is_error(jobj)) {
		output_json(jobj, data_outlet, done_outlet);
		json_object_put(jobj);
	} else {
		error("Not a JSON object");
	}
	freebytes(corrected_json_string, memsize);
}

char *remove_backslashes(char *source_string, size_t memsize) {
	char *dest = NULL;
	char remove[2] = "\\,";
	int found;
	size_t i = 0;
	size_t j = 0;
	size_t len_src = strlen(source_string);

	memsize = (len_src + 1) * sizeof(char);
	
	dest = (char *) getbytes(memsize * sizeof(char));
	if (dest == NULL) {
		error("Unable to allocate memory\n");
	}

	for (i = 0; i < memsize; i++ ) {
		found = FALSE;
		if (source_string[i] == remove[0] && source_string[i + 1] == remove[1]) {
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
