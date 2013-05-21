struct _kvp {
	size_t key_len;
	char *key;
	size_t value_len;
	char *value;
	unsigned char is_array;
	struct _kvp *next;
};

struct _kvp_store {
	t_object x_ob;
	struct _kvp *first_data;
	struct _kvp *last_data;
	size_t data_count;
};

static struct _kvp *kvp_create(char *key, char *value, unsigned char is_array){
	struct _kvp *created_data = NULL;

	created_data = getbytes(sizeof(struct _kvp));
	created_data->key = string_create(&created_data->key_len, strlen(key));
	created_data->value = string_create(&created_data->value_len, strlen(value));
	if (created_data == NULL || key == NULL || value == NULL) {
		MYERROR("Could not get data");
		return NULL;
	}

	strcpy(created_data->key, key);
	strcpy(created_data->value, value);
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

static void kvp_add(struct _kvp_store *x, char *key, char *value, unsigned char is_array) {
	struct _kvp *compare = x->first_data;
	struct _kvp *existing = NULL;
	struct _kvp *new = NULL;
	if (!is_array) {
		while (compare != NULL) {
			if (strcmp(compare->key, key) == 0) {
				existing = compare;
				break;
			}
			compare = compare->next;
		}
	}
	if (existing != NULL) {
		string_free(existing->value, &existing->value_len);
		existing->value = string_create(&existing->value_len, strlen(value));
		strcpy(existing->value, value);
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
		string_free(data_to_free->value, &data_to_free->value_len);
		freebytes(data_to_free, sizeof(struct _kvp));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->first_data = NULL;
	x->last_data = NULL;
}
