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

struct _jenc_json_array {
	size_t *numbers;
	size_t count;
	json_object **members;
};

static json_object *jenc_create_object(struct _v *value);
static void jenc_load_json_object(t_json_encode *jenc, json_object *jobj);
static void jenc_load_json_data(t_json_encode *jenc, json_object *jobj);
static json_object *jenc_get_array_value(struct _kvp **item);
static struct _jenc_json_array *jenc_create_array(size_t count);
static void jenc_free_array(struct _jenc_json_array *arr, size_t count);
static t_symbol *jenc_get_json_symbol(t_json_encode *jenc);
static void jenc_add(t_json_encode *jenc, int argc, t_atom *argv, unsigned char is_array);

/* begin implementations */
static json_object *jenc_create_object(struct _v *value) {
	json_object *object;
	/* if stored value is string is starting with { and ending with }, 
	   then create a json object from it. */
	if (value->slen == 0){
		object = json_object_new_double(value->val.f);
	}
	else if (value->val.s[0] == '{' && value->val.s[strlen(value->val.s) - 1] == '}') {
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

static void jenc_load_json_object(t_json_encode *jenc, json_object *jobj) {
	json_object_object_foreach(jobj, key, val) {
		char *value;
		size_t value_len = 0;
		int array_len;
		enum json_type inner_type = json_object_get_type(val);

		switch (inner_type) {
			case json_type_boolean:
				kvp_add((struct _kvp_store *)jenc, key, 
						kvp_val_create(NULL, json_object_get_boolean(val) ? 1 : 0), 0);
				break;
			case json_type_double:
				kvp_add((struct _kvp_store *)jenc, key, 
						kvp_val_create(NULL, json_object_get_double(val)), 0);
				break;
			case json_type_int:
				kvp_add((struct _kvp_store *)jenc, key, 
						kvp_val_create(NULL, json_object_get_int(val)), 0);
				break;
			case json_type_string:
				value = string_create(&value_len, snprintf(NULL, 0, "%s", json_object_get_string(val)));
				sprintf(value, "%s", json_object_get_string(val));
				kvp_add((struct _kvp_store *)jenc, key, kvp_val_create(value, 0), 0);
				string_free(value, &value_len);
				break;
			case json_type_object:
				value = string_create(&value_len, snprintf(NULL, 0, "%s", json_object_get_string(val)));
				sprintf(value, "%s", json_object_get_string(val));
				kvp_add((struct _kvp_store *)jenc, key, kvp_val_create(value, 0), 0);
				string_free(value, &value_len);
				json_object_put(val);
				break;
			case json_type_array:
				array_len = json_object_array_length(val);
				for (int i = 0; i < array_len; i++) {
					json_object *array_member = json_object_array_get_idx(val, i);
					if (!is_error(array_member)) {
						value = string_create(&value_len, 
								snprintf(NULL, 0, "%s", json_object_get_string(array_member)));
						sprintf(value, "%s", json_object_get_string(array_member));
						kvp_add((struct _kvp_store *)jenc, key, kvp_val_create(value, 0), 0);
						string_free(value, &value_len);
					}
				}
				break;
			case json_type_null:
				kvp_add((struct _kvp_store *)jenc, key, kvp_val_create("", 0), 0);
				break;
		}
	}
}

static void jenc_load_json_data(t_json_encode *jenc, json_object *jobj) {
	enum json_type type;

	kvp_store_free_memory((struct _kvp_store *)jenc);
	type = json_object_get_type(jobj);

	switch (type) {
		case json_type_object:
			jenc_load_json_object(jenc, jobj);
			break;
		default: 
			pd_error(jenc, "This JSON data cannot be represented internally, sorry");
			break;
	}
}

static json_object *jenc_get_array_value(struct _kvp **item) {
	struct _kvp *it = *item;
	char *key = it->key;
	json_object *value = json_object_new_array();
	json_object *array_member;

	array_member = jenc_create_object(it->value);
	json_object_array_add(value, array_member);

	while (it->next != NULL && strcmp(it->next->key, key) == 0) {
		it = it->next;
		array_member = jenc_create_object(it->value);
		json_object_array_add(value, array_member);
	}	

	*item = it;
	return value;
}

static struct _jenc_json_array *jenc_create_array(size_t count) {
	struct _jenc_json_array *arr = getbytes(sizeof(struct _jenc_json_array));

	if (arr == NULL) {
		MYERROR("not enough memory");
		return arr;
	}
	arr->numbers = getbytes(count * sizeof(size_t));
	arr->members = getbytes(count * sizeof(json_object *));
	return arr;
}

static void jenc_free_array(struct _jenc_json_array *arr, size_t count) {
	size_t i;

	for (i = 0; i < arr->count; i++) {
		json_object_put(arr->members[i]);
	}
	freebytes(arr->numbers, count * sizeof(size_t));
	freebytes(arr->members, count * sizeof(json_object *));
	freebytes(arr, sizeof(struct _jenc_json_array));
}

static t_symbol *jenc_get_json_symbol(t_json_encode *jenc) {
	struct _jenc_json_array *arr_members = jenc_create_array(jenc->storage.data_count);
	struct _kvp *it;
	json_object *jobj = json_object_new_object();
	json_object *value;
	t_symbol *json_symbol = NULL;

	if (jenc->storage.data_count == 0) {
		json_symbol = gensym("");
		return json_symbol;
	}

	it = jenc->storage.first_data;
	while(it) {
		if (it->is_array == 1) {
			value = jenc_get_array_value(&it);
		} else {
			value = jenc_create_object(it->value);
		}
		json_object_object_add(jobj, it->key, value); 
		it = it->next;
	}
	json_symbol = gensym(json_object_to_json_string(jobj));
	json_object_put(jobj);
	jenc_free_array(arr_members, jenc->storage.data_count);
	return json_symbol;
}

static void jenc_add(t_json_encode *jenc, int argc, t_atom *argv, unsigned char is_array) {
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
	kvp_add((struct _kvp_store *)jenc, key, kvp_val_create(value, f), is_array);
	string_free(value, &value_len);
}

void json_encode_add(t_json_encode *jenc, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;

	jenc_add(jenc, argc, argv, 0);
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

void *json_encode_new(t_symbol *sel, int argc, t_atom *argv) {
	t_json_encode *jenc = (t_json_encode *)pd_new(json_encode_class);

	(void) sel;
	(void) argc;
	(void) argv;

	jenc->storage.data_count = 0;
	outlet_new(&jenc->storage.x_ob, NULL);
	jenc->x_canvas = canvas_getcurrent();
	return (void *)jenc;
}

void json_encode_free (t_json_encode *jenc, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)jenc);
}

void json_encode_bang(t_json_encode *jenc) {
	outlet_symbol(jenc->storage.x_ob.ob_outlet, jenc_get_json_symbol(jenc));
}

void json_encode_array(t_json_encode *jenc, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;

	jenc_add(jenc, argc, argv, 1);
}

void json_encode_read(t_json_encode *jenc, t_symbol *filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	struct stat st;
	char *json_string;
	json_object *jobj;

	canvas_makefilename(jenc->x_canvas, filename->s_name, buf, MAXPDSTRING);
	file = fopen(buf, "r");
	if (!file) {
		pd_error(jenc, "%s: read failed", filename->s_name);
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
		jenc_load_json_data(jenc, jobj);
		json_object_put(jobj);
	} else {
		post("File does not contain valid JSON.");
	}
}

void json_encode_write(t_json_encode *jenc, t_symbol *filename) {
	char buf[MAXPDSTRING];
	FILE *file = NULL;
	t_symbol *json_symbol = jenc_get_json_symbol(jenc);
	char *json_string = json_symbol->s_name;

	if (!json_string) {
		post("No JSON data for writing available.");
		return;
	}

	canvas_makefilename(jenc->x_canvas, filename->s_name, buf, MAXPDSTRING);
	if ((file = fopen(buf, "w"))) {
		fprintf(file, json_string);
		fclose(file);
	} else {
		pd_error(jenc, "%s: write failed", filename->s_name);
	}
}

void json_encode_clear(t_json_encode *jenc, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	kvp_store_free_memory((struct _kvp_store *)jenc);
}
