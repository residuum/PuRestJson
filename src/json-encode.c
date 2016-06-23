/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2015 Thomas Mayer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

/*
 * [json-encodes] encodes data as JSON and outputs it as a symbol.
 * */

#include "json-encode.h"
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>

#include "uthash.h"
#include "inc/string.c"
#include "inc/kvp.c"

static t_class *json_encode_class;

struct _json_encode {
	struct _kvp_store storage;
	t_canvas *x_canvas; /* needed for getting file names */
};

/* gets json object */
static json_object *jenc_create_object(const struct _v *value);
/* loads json object */
static void jenc_load_json_object(const t_json_encode *jenc, json_object *jobj);
/* loads json data */
static void jenc_load_json_data(t_json_encode *jenc, json_object *jobj);
/* gets json array from key value pair */
static json_object *jenc_get_array_value(struct _kvp *item);
/* creates a json string from store */
static t_symbol *jenc_get_json_symbol(t_json_encode *jenc);
/* adds item to store */
static void jenc_add(t_json_encode *jenc, const int argc, t_atom *argv, const unsigned char is_array);

/* begin implementations */
static json_object *jenc_create_object(const struct _v *const value) {
	json_object *object;
	if (value->type == float_val) {
		object = json_object_new_double(value->val.f);
	} else if (value->type == int_val) {
		object = json_object_new_int(value->val.i);
		/* if stored value is string is starting with { and ending with }, 
		   then create a json object from it. */
	} else if (value->val.s[0] == '{' && value->val.s[strlen(value->val.s) - 1] == '}') {
		char *parsed_string;
		size_t memsize = 0;
		parsed_string = string_remove_backslashes(value->val.s, &memsize);
		object = json_tokener_parse(parsed_string);
		string_free(parsed_string, &memsize);
	} else {
		object = json_object_new_string(value->val.s);
	}
	return object;
}

static void jenc_load_json_object(const t_json_encode *const jenc, json_object *const jobj) {
	json_object_object_foreach(jobj, key, val) {
		char *value;
		size_t value_len = 0;
		int array_len;
		const enum json_type inner_type = json_object_get_type(val);

		switch (inner_type) {
			case json_type_boolean:
				kvp_add_simple((struct _kvp_store *)jenc, key, 
						kvp_val_create(NULL, json_object_get_boolean(val) ? 1 : 0));
				break;
			case json_type_double:
				kvp_add_simple((struct _kvp_store *)jenc, key, 
						kvp_val_create(NULL, json_object_get_double(val)));
				break;
			case json_type_int:
				kvp_add_simple((struct _kvp_store *)jenc, key, 
						kvp_val_create(NULL, json_object_get_int(val)));
				break;
			case json_type_string:
				value = string_create(&value_len, snprintf(NULL, 0, "%s", 
							json_object_get_string(val)));
				sprintf(value, "%s", json_object_get_string(val));
				kvp_add_simple((struct _kvp_store *)jenc, key, kvp_val_create(value, 0));
				string_free(value, &value_len);
				break;
			case json_type_object:
				value = string_create(&value_len, snprintf(NULL, 0, "%s", 
							json_object_get_string(val)));
				sprintf(value, "%s", json_object_get_string(val));
				kvp_add_simple((struct _kvp_store *)jenc, key, kvp_val_create(value, 0));
				string_free(value, &value_len);
				break;
			case json_type_array:
				array_len = json_object_array_length(val);
				for (int i = 0; i < array_len; i++) {
					json_object *array_member = json_object_array_get_idx(val, i);
					if (!is_error(array_member)) {
						value = string_create(&value_len, 
								snprintf(NULL, 0, "%s",
									json_object_get_string(array_member)));
						sprintf(value, "%s", json_object_get_string(array_member));
						kvp_add_array((struct _kvp_store *)jenc, key, 
								kvp_val_create(value, 0));
						string_free(value, &value_len);
					}
				}
				break;
			case json_type_null:
				kvp_add_simple((struct _kvp_store *)jenc, key, kvp_val_create("", 0));
				break;
			default:
				MYERROR("What other JSON type?");
				break;
		}
	}
}

static void jenc_load_json_data(t_json_encode *const jenc, json_object *const jobj) {
	const enum json_type type = json_object_get_type(jobj);

	kvp_store_free_memory((struct _kvp_store *)jenc);
	switch (type) {
		case json_type_object:
			jenc_load_json_object(jenc, jobj);
			break;
		default: 
			pd_error(jenc, "This JSON data cannot be represented internally, sorry.");
			break;
	}
}

static json_object *jenc_get_array_value(struct _kvp *item) {
	struct _v *value = item->value;
	json_object *json_value = json_object_new_array();
	json_object *array_member;

	array_member = jenc_create_object(value);
	json_object_array_add(json_value, array_member);

	while (value->next != NULL) {
		value = value->next;
		array_member = jenc_create_object(value);
		json_object_array_add(json_value, array_member);
	}	

	return json_value;
}

static t_symbol *jenc_get_json_symbol(t_json_encode *const jenc) {
	struct _kvp *it;
	json_object *const jobj = json_object_new_object();
	json_object *value;
	t_symbol *json_symbol = NULL;

	if (!HASH_COUNT(jenc->storage.data)) {
		json_symbol = gensym("");
		return json_symbol;
	}

	for (it = jenc->storage.data; it != NULL; it = it->hh.next) {
		if (it->is_array == 1) {
			value = jenc_get_array_value(it);
		} else {
			value = jenc_create_object(it->value);
		}
		json_object_object_add(jobj, it->key, value); 
	}
	json_symbol = gensym(json_object_to_json_string(jobj));
	json_object_put(jobj);
	return json_symbol;
}

static void jenc_add(t_json_encode *const jenc, const int argc, t_atom *const argv, const unsigned char is_array) {
	char key[MAXPDSTRING];
	size_t value_len = 0;
	char *value = NULL;
	t_float f = 0;

	if (argc < 2) {
		pd_error(jenc, "For method '%s' You need to specify a value.", is_array ? "array": "add");
		return;
	}

	atom_string(argv, key, MAXPDSTRING);
	/* Special case: only 2 arguments, and 2nd argument is a number */
	if (argc == 2 && (argv + 1)->a_type == A_FLOAT) {
		f = atom_getfloat(argv + 1);
	} else {
		char temp_value[MAXPDSTRING];
		for (int i = 1; i < argc; i++) {
			atom_string(argv + i, temp_value, MAXPDSTRING);
			value_len += strlen(temp_value) + 1;
		}
		value = getbytes(value_len * sizeof(char));
		atom_string(argv + 1, value, MAXPDSTRING);
		for(int i = 2; i < argc; i++) {
			atom_string(argv + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
	}
	if (is_array){
		kvp_add_array((struct _kvp_store *)jenc, key, kvp_val_create(value, f));
	} else {
		kvp_add_simple((struct _kvp_store *)jenc, key, kvp_val_create(value, f));
	}
	string_free(value, &value_len);
}

void setup_json0x2dencode(void) {
	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
			(t_method)json_encode_free, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_array, gensym("array"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_read, gensym("read"), A_SYMBOL, A_DEFSYM, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_write, gensym("write"), A_SYMBOL, A_DEFSYM, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
	class_sethelpsymbol(json_encode_class, gensym("json"));
}

void json_encode_free (t_json_encode *const jenc, const t_symbol *const sel, const int argc, 
		const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)jenc);
}

void json_encode_bang(t_json_encode *const jenc) {
	outlet_symbol(jenc->storage.x_ob.ob_outlet, jenc_get_json_symbol(jenc));
}

void json_encode_add(t_json_encode *const jenc, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	jenc_add(jenc, argc, argv, 0);
}

void json_encode_array(t_json_encode *const jenc, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	jenc_add(jenc, argc, argv, 1);
}

void json_encode_read(t_json_encode *const jenc, const t_symbol *const filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	struct stat st;
	char *json_string;
	json_object *jobj;
	size_t file_size;

	canvas_makefilename(jenc->x_canvas, filename->s_name, buf, MAXPDSTRING);
	file = fopen(buf, "r");
	if (file == NULL) {
		pd_error(jenc, "%s: read failed.", filename->s_name);
		return;
	}
	if (stat(buf, &st) == -1) {
		pd_error(jenc, "%s: not a regular file.", filename->s_name);
		fclose(file);
		return;
	}
	json_string = getbytes((st.st_size + 1) * sizeof(char));
	json_string[st.st_size] = 0x00;
	file_size = fread(json_string, sizeof(char), st.st_size, file);
	fclose(file);
	if (file_size != (size_t)st.st_size) {
		pd_error(jenc, "%s: file size could not be determined", filename->s_name);
		return;
	}
	jobj = json_tokener_parse(json_string);
	freebytes(json_string, (st.st_size + 1) * sizeof(char));
	if (!is_error(jobj)) {
		jenc_load_json_data(jenc, jobj);
		json_object_put(jobj);
	} else {
		post("File does not contain valid JSON.");
	}
}

void json_encode_write(t_json_encode *const jenc, const t_symbol *const filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	const t_symbol *const json_symbol = jenc_get_json_symbol(jenc);
	const char *const json_string = json_symbol->s_name;

	if (json_string == NULL) {
		post("No JSON data for writing available.");
		return;
	}

	canvas_makefilename(jenc->x_canvas, filename->s_name, buf, MAXPDSTRING);
	if ((file = fopen(buf, "w"))) {
		fprintf(file, "%s", json_string);
		fclose(file);
	} else {
		pd_error(jenc, "%s: write failed.", filename->s_name);
	}
}

void *json_encode_new(const t_symbol *const sel, const int argc, const t_atom *const argv) {
	t_json_encode *const jenc = (t_json_encode *)pd_new(json_encode_class);

	(void) sel;
	(void) argc;
	(void) argv;

	outlet_new(&jenc->storage.x_ob, NULL);
	jenc->x_canvas = canvas_getcurrent();
	purest_json_lib_info("json-encode");
	return (void *)jenc;
}

void json_encode_clear(t_json_encode *const jenc, const t_symbol *const sel, const int argc, 
		const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)jenc);
}
