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
	struct _kvp *data;
};

static struct _v *kvp_val_create(const char *s, const t_float f);
static struct _v *kvp_val_free(struct _v *value);
static struct _kvp *kvp_create(const char *key, struct _v *value, const unsigned char is_array);
static void kvp_free(struct _kvp *item);
static void kvp_insert(struct _kvp_store *store, struct _kvp *new_pair);
static void kvp_replace_value(struct _kvp *kvp, struct _v *value, const unsigned char is_array);
static void kvp_add_simple(struct _kvp_store *store, char *key, struct _v *value);
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

static struct _v *kvp_val_free(struct _v *const value) {
	struct _v *next = value->next;
	string_free(value->val.s, &value->slen);
	freebytes(value, sizeof(struct _v));
	return next;
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
	struct _v *value = item->value;
	do {
		value = kvp_val_free(value);
	} while (value != NULL);
	freebytes(item, sizeof(struct _kvp));
}

static void kvp_insert(struct _kvp_store *const store, struct _kvp *const new_pair) {
	if (new_pair == NULL) {
		return;
	}
	HASH_ADD_KEYPTR(hh, store->data, new_pair->key, new_pair->key_len - 1, new_pair);
}

static void kvp_replace_value(struct _kvp *kvp, struct _v *const value, const unsigned char is_array) {
		struct _v *existing = kvp->value;
		do {
			existing = kvp_val_free(existing);
		} while(existing != NULL);
		kvp->value = value;
		kvp->is_array = is_array;
}

static void kvp_add_simple(struct _kvp_store *const store, char *const key, struct _v *const value) {
	struct _kvp *new;
	HASH_FIND_STR(store->data, key, new);
	if (new != NULL) {
		kvp_replace_value(new, value, 0);
	} else {
		new = kvp_create(key, value, 0);
		kvp_insert(store, new);
	}
}

static void kvp_add_array(struct _kvp_store *const store, char *const key, struct _v *const value) {
	struct _kvp *new;
	HASH_FIND_STR(store->data, key, new);
	if (new != NULL) {
		if (new->is_array) {
			struct _v *last = new->last;
			last->next = value;
			new->last = value;
		} else {
			kvp_replace_value(new, value, 1);
		}
	} else {
		new = kvp_create(key, value, 1);
		kvp_insert(store, new);
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
