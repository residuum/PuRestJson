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
static void kvp_store_add(struct _kvp_store *x, struct _kvp *new_pair);
static void kvp_add(struct _kvp_store *x, char *key, struct _v *value, unsigned char is_array);
static void kvp_store_free_memory(struct _kvp_store *x);

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

static void kvp_store_add(struct _kvp_store *x, struct _kvp *new_pair) {
	if (!new_pair) {
		return;
	}

	x->data_count++;
	if (!x->first_data) {
		x->first_data = new_pair;
	} else {
		x->last_data->next = new_pair;
	}
	x->last_data = new_pair;
}

static void kvp_add(struct _kvp_store *x, char *key, struct _v *value, unsigned char is_array) {
	struct _kvp *compare = x->first_data;
	struct _kvp *existing = NULL;
	struct _kvp *new = NULL;

	if (is_array == 0) {
		while (compare != NULL) {
			if (strcmp(compare->key, key) == 0) {
				existing = compare;
				break;
			}
			compare = compare->next;
		}
	}
	if (existing != NULL) {
		kvp_val_free(existing->value);
		existing->value = value;
	} else {
		new = kvp_create(key, value, is_array);
		kvp_store_add(x, new);
	}
}

static void kvp_store_free_memory(struct _kvp_store *x) {
	struct _kvp *data_to_free;
	struct _kvp *next_data;

	data_to_free = x->first_data;
	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		string_free(data_to_free->key, &data_to_free->key_len);
		kvp_val_free(data_to_free->value);
		freebytes(data_to_free, sizeof(struct _kvp));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->first_data = NULL;
	x->last_data = NULL;
}
