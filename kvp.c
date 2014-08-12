enum _v_type {string_val, float_val, int_val};

struct _v {
	size_t slen;
	enum _v_type type;
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
	unsigned char is_array;
	struct _kvp *next;
};

/*
 * Data in the linked list:
 * 1. Sorted by addition
 * 2. Array members are directly following 
 *
 * Consider the following succession:
 * add id 0, array_add member 1, array_add member 2, id 3, add name test, array_add member 4
 *
 * yields after each step:
 * id 0
 * id 0, member 1
 * id 0, member 1, member 2
 * id 3, member 1, member 2
 * id 3, member 1, member 2, name test
 * id 3, member 1, member 2, name test
 * id 3, member 1, member 2, member 4, name test
 */

struct _kvp_store {
	t_object x_ob;
	struct _kvp *first_data;
	struct _kvp *last_data;
};

static struct _v *kvp_val_create(const char *s, const t_float f);
static void kvp_val_free(struct _v *value);
static struct _kvp *kvp_create(const char *key, struct _v *value, const unsigned char is_array);
static void kvp_free(struct _kvp *item);
static void kvp_insert(struct _kvp_store *store, struct _kvp *after, struct _kvp *new_pair);
static void kvp_replace_single(struct _kvp *item, struct _v *value, const unsigned char is_array);
static void kvp_replace_array(struct _kvp *item, const char *key, struct _v *value);
static void kvp_add_array(struct _kvp_store *store, struct _kvp *item, char *key, struct _v *value);
static void kvp_add(struct _kvp_store *store, char *key, struct _v *value, const unsigned char is_array);
static void kvp_store_free_memory(struct _kvp_store *store);

/* begin implementations */
static struct _v *kvp_val_create(const char *const s, const t_float f) {
	struct _v *created = NULL;

	created = getbytes(sizeof(struct _v));
	created->slen = 0;
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

static void kvp_val_free(struct _v *const value) {
	string_free(value->val.s, &value->slen);
	freebytes(value, sizeof(struct _v));
}

static struct _kvp *kvp_create(const char *const key, struct _v *const value, const unsigned char is_array) {
	struct _kvp *created_data = NULL;

	created_data = getbytes(sizeof(struct _kvp));
	created_data->key = string_create(&created_data->key_len, strlen(key));
	if (created_data == NULL || key == NULL || value == NULL) {
		MYERROR("Could not get data");
		return NULL;
	}

	created_data->value = value;
	strcpy(created_data->key, key);
	created_data->next = NULL;
	created_data->is_array = is_array;

	return created_data;
}

static void kvp_free(struct _kvp *const item) {
	string_free(item->key, &item->key_len);
	kvp_val_free(item->value);
	freebytes(item, sizeof(struct _kvp));
}

static void kvp_insert(struct _kvp_store *const store, struct _kvp *const after, struct _kvp *const new_pair) {
	if (new_pair == NULL) {
		return;
	}
	if (after == NULL) {
		store->first_data = new_pair;
	} else {
		new_pair->next = after->next;
		after->next = new_pair;
	}
	if (new_pair->next == NULL) {
		store->last_data = new_pair;
	}
}

static void kvp_replace_single(struct _kvp *const item, struct _v *const value, const unsigned char is_array) {
	kvp_val_free(item->value);
	item->value = value;
	item->is_array = is_array;
}

static void kvp_replace_array(struct _kvp *const item, const char *const key, struct _v *const value) {
	struct _kvp *to_free;

	kvp_val_free(item->value);
	item->value = value;
	item->is_array = 0;
	while (item->next != NULL && strcmp(item->next->key, key) == 0) {
		to_free = item->next;
		item->next = to_free->next;
		kvp_free(to_free);
	}	
}

static void kvp_add_array(struct _kvp_store *const store, struct _kvp *const item, char *const key, struct _v *const value) {
	struct _kvp *it = item;
	struct _kvp *new;

	while (it->next != NULL && strcmp(it->next->key, key) == 0) {
		it = it->next;
	}
	new = kvp_create(key, value, 1);
	kvp_insert(store, it, new);
}

static void kvp_add(struct _kvp_store *const store, char *const key, struct _v *const value, const unsigned char is_array) {
	struct _kvp *it = store->first_data;
	unsigned char found = 0;

	while (it != NULL) {
		if (strcmp(it->key, key) == 0) {
			found = 1;
			if (!it->is_array) {
				kvp_replace_single(it, value, is_array);
			} else if (!is_array) {
				kvp_replace_array(it, key, value);
			} else {
				kvp_add_array(store, it, key, value);
			}
			break;
		}
		it = it->next;
	}

	if (!found) {
		struct _kvp *new = kvp_create(key, value, is_array);
		kvp_insert(store, store->last_data, new);
	}
}

static void kvp_store_free_memory(struct _kvp_store *const store) {
	struct _kvp *data_to_free;

	data_to_free = store->first_data;
	while(data_to_free != NULL) {
		struct _kvp *next_data = data_to_free->next;
		kvp_free(data_to_free);
		data_to_free = next_data;
	}

	store->first_data = NULL;
	store->last_data = NULL;
}
