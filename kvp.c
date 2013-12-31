struct _v {
	size_t slen;
	union {
		t_float f;
		char *s;
	} val;
};

struct _kvp {
	size_t key_len;
	char *key;
	struct _v *value;
	unsigned char is_array;
	struct _kvp *next;
};

struct _kvp_store {
	t_object x_ob;
	struct _kvp *first_data;
	struct _kvp *last_data;
	size_t data_count;
};

static struct _v *kvp_val_create(char *s, t_float f);
static void kvp_val_free(struct _v *value);
static struct _kvp *kvp_create(char *key, struct _v *value, unsigned char is_array);
static void kvp_free(struct _kvp *item);
static void kvp_insert(struct _kvp_store *store, struct _kvp *after, struct _kvp *new_pair);
static void kvp_remove(struct _kvp_store *store, struct _kvp *item);
static void kvp_add(struct _kvp_store *store, char *key, struct _v *value, unsigned char is_array);
static void kvp_store_free_memory(struct _kvp_store *store);

/* begin implementations */
static struct _v *kvp_val_create(char *s, t_float f) {
	struct _v *created = NULL;

	created = getbytes(sizeof(struct _v));
	created->slen = 0;
	if (s) {
		created->val.s = string_create(&created->slen, strlen(s));
		strcpy(created->val.s, s); 
	} else {
		created->val.f = f;
	}
	return created;
}

static void kvp_val_free(struct _v *value) {
	string_free(value->val.s, &value->slen);
	freebytes(value, sizeof(struct _v));
}

static struct _kvp *kvp_create(char *key, struct _v *value, unsigned char is_array) {
>>>>>>> 2a128e1... [json-encode]: Add numbers correctly, not as string.
	struct _kvp *created_data = NULL;

	created_data = getbytes(sizeof(struct _kvp));
	created_data->key = string_create(&created_data->key_len, strlen(key));
	created_data->value = value;
	if (created_data == NULL || key == NULL || value == NULL) {
		MYERROR("Could not get data");
		return NULL;
	}

	strcpy(created_data->key, key);
	created_data->next = NULL;
	created_data->is_array = is_array;

	return created_data;
}

static void kvp_free(struct _kvp *item) {
	string_free(item->key, &item->key_len);
	kvp_val_free(item->value);
	freebytes(item, sizeof(struct _kvp));
}

static void kvp_insert(struct _kvp_store *store, struct _kvp *after, struct _kvp *new_pair) {
	if (!new_pair) {
		return;
	}

	store->data_count++;
	if (!after) {
		store->first_data = new_pair;
	} else {
		new_pair->next = after->next;
		after->next = new_pair;
	}
	if (!new_pair->next){
		store->last_data = new_pair;
	}
}

static void kvp_remove(struct _kvp_store *store, struct _kvp *item) {
	struct _kvp *it = store->first_data;

	while(it != NULL) {
		if (it->next == item) {
			it->next = item->next;
			kvp_free(item);
			store->data_count--;
			if (it->next == NULL) {
				store->last_data = it;
			}
		}
		it = it->next;
	}
}

static void kvp_add(struct _kvp_store *store, char *key, struct _v *value, unsigned char is_array) {
	struct _kvp *it = store->first_data;
	struct _kvp *new = NULL;
	unsigned char found = 0;
	struct _kvp *remove[store->data_count];

	while (it != NULL) {
		if (strcmp(it->key, key) == 0) {
			found++;
			/* iterator not in array, only value to replace */
			if (!it->is_array) {
				kvp_val_free(it->value);
				it->value = value;
				it->is_array = is_array;
				break;
			/* iterator in array, replace first, remove others */
			} else if (!is_array) {
				if (found == 1) {
					kvp_val_free(it->value);
					it->value = value;
					it->is_array = is_array;
				} else {
					remove[found - 2] = it;
				}
			/* insert after last one */
			} else if (!it->next || strcmp(it->next->key, key) != 0) {
				new = kvp_create(key, value, is_array);
				kvp_insert(store, it, new);
				found = 1;
				break;
			}
		}
		it = it->next;
	}

	if (!found) {
		new = kvp_create(key, value, is_array);
		kvp_insert(store, store->last_data, new);
	} else {
		for (int i = 0; i < found - 1; i++) {
			kvp_remove(store, remove[i]);
		}
	}
}

static void kvp_store_free_memory(struct _kvp_store *store) {
	struct _kvp *data_to_free;

	data_to_free = store->first_data;
	while(data_to_free != NULL) {
		struct _kvp *next_data = data_to_free->next;
		kvp_free(data_to_free);
		data_to_free = next_data;
	}

	store->data_count = 0;
	store->first_data = NULL;
	store->last_data = NULL;
}
