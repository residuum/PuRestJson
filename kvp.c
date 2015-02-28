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

enum _v_type {string_val, float_val, int_val};

struct _v {
	size_t slen;
	enum _v_type type;
	struct _v *next;
	union {
		t_float f;
		char *s;
		int i;
	} val;
};

struct _kvp {
	size_t key_len;
	char *key;
	struct _v *value;
	struct _v *last;
	unsigned char is_array;
	UT_hash_handle hh;
};

struct _kvp_store {
	t_object x_ob;
	struct _kvp *data;
};

static struct _v *kvp_val_create(const char *s, const t_float f);
static void kvp_val_free(struct _v *value);
static struct _kvp *kvp_create(const char *key, struct _v *value, const unsigned char is_array);
static void kvp_free(struct _kvp *item);
static void kvp_insert(struct _kvp_store *store, struct _kvp *new_pair);
static void kvp_replace_value(struct _kvp *kvp, struct _v *value, const unsigned char is_array);
static void kvp_add_simple(struct _kvp_store *store, char *key, struct _v *value);
static void kvp_add_to_array(struct _kvp *kvp, struct _v *value);
static void kvp_add_array(struct _kvp_store *store, char *key, struct _v *value);
static void kvp_add(struct _kvp_store *store, char *key, struct _v *value, const unsigned char is_array);
static void kvp_store_free_memory(struct _kvp_store *store);

/* begin implementations */
static struct _v *kvp_val_create(const char *const s, const t_float f) {
	struct _v *created = NULL;

	created = getbytes(sizeof(struct _v));
	created->slen = 0;
	created->next = NULL;
	if (s) {
		created->val.s = string_create(&created->slen, strlen(s));
		strcpy(created->val.s, s); 
		created->type = string_val;
	} else {
		double intpart;
		if (modf((double)f, &intpart) == 0){
			created->val.i = (int)intpart;
			created->type = int_val;	
		} else {
			created->val.f = f;
			created->type = float_val;
		}
	}
	return created;
}

static void kvp_val_free(struct _v *value) {
	do {
		struct _v *next = value->next;
		string_free(value->val.s, &value->slen);
		freebytes(value, sizeof(struct _v));
		value = next;
	} while (value != NULL);
}

static struct _kvp *kvp_create(const char *const key, struct _v *const value, const unsigned char is_array) {
	struct _kvp *created_data = NULL;

	created_data = getbytes(sizeof(struct _kvp));
	created_data->key = string_create(&created_data->key_len, strlen(key));
	if (created_data == NULL || key == NULL || value == NULL) {
		MYERROR("Could not get data.");
		return NULL;
	}

	created_data->value = value;
	created_data->last = value;
	strcpy(created_data->key, key);
	created_data->is_array = is_array;

	return created_data;
}

static void kvp_free(struct _kvp *const item) {
	string_free(item->key, &item->key_len);
	kvp_val_free(item->value);
	freebytes(item, sizeof(struct _kvp));
}

static void kvp_insert(struct _kvp_store *const store, struct _kvp *const new_pair) {
	MYASSERT(new_pair != NULL, "New pair is null.");

	if (new_pair == NULL) {
		return;
	}
	/* new_pair->key_len = strlen(new_pair->key) + 1, see string.c */
	HASH_ADD_KEYPTR(hh, store->data, new_pair->key, new_pair->key_len - 1, new_pair);
}

static void kvp_replace_value(struct _kvp *const kvp, struct _v *const value, const unsigned char is_array) {
	MYASSERT(kvp->is_array != 1 || is_array != 1, "This should not be called: array values should be appended, not replaced.");
	
	kvp_val_free(kvp->value);
	kvp->value = value;
	kvp->is_array = is_array;
}

static void kvp_add_simple(struct _kvp_store *const store, char *const key, struct _v *const value) {
	struct _kvp *kvp;

	HASH_FIND_STR(store->data, key, kvp);
	if (kvp != NULL) {
		kvp_replace_value(kvp, value, 0);
	} else {
		kvp = kvp_create(key, value, 0);
		kvp_insert(store, kvp);
	}
}

static void kvp_add_to_array(struct _kvp *const kvp, struct _v *const value) {
	struct _v *last = kvp->last;

	last->next = value;
	kvp->last = value;
}

static void kvp_add_array(struct _kvp_store *const store, char *const key, struct _v *const value) {
	struct _kvp *kvp;

	HASH_FIND_STR(store->data, key, kvp);
	if (kvp != NULL) {
		if (kvp->is_array) {
			kvp_add_to_array(kvp, value);
		} else {
			kvp_replace_value(kvp, value, 1);
		}
	} else {
		kvp = kvp_create(key, value, 1);
		kvp_insert(store, kvp);
	}
}

static void kvp_add(struct _kvp_store *const store, char *const key, struct _v *const value, 
		const unsigned char is_array) {
	if (!is_array) {
		kvp_add_simple(store, key, value);
	} else {
		kvp_add_array(store, key, value);
	}
}

static void kvp_store_free_memory(struct _kvp_store *const store) {
	struct _kvp *it;
	struct _kvp *tmp;

	HASH_ITER(hh, store->data, it, tmp) {
		HASH_DEL(store->data, it);
		kvp_free(it);
	}
	store->data = NULL;
}
