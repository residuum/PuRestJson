struct _key_value_pair {
	size_t key_len;
	char *key;
	size_t value_len;
	char *value;
	short is_array;
	struct _key_value_pair *next;
};

struct _kvp_storage {
	t_object x_ob;
	struct _key_value_pair *first_data;
	struct _key_value_pair *last_data;
	int data_count;
};

static struct _key_value_pair *create_key_value_pair(char *key, char *value, int is_array){
	struct _key_value_pair *created_data = NULL;

	created_data = getbytes(sizeof(struct _key_value_pair));
	created_data->key_len = strlen(key) + 1;
	created_data->value_len = strlen(value) + 1;
	created_data->key = getbytes(created_data->key_len * sizeof(char));
	created_data->value = getbytes(created_data->value_len * sizeof(char));
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

static void kvp_storage_add(struct _kvp_storage *x, struct _key_value_pair *new_pair) {
	if (new_pair) {
		x->data_count++;
		if (!x->first_data) {
			x->first_data = new_pair;
		} else {
			x->last_data->next = new_pair;
		}
		x->last_data = new_pair;
	}
}

static void kvp_storage_free_memory(struct _kvp_storage *x) {
	struct _key_value_pair *data_to_free;
	struct _key_value_pair *next_data;

	data_to_free = x->first_data;
	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		/* TODO: Investigate the reason for segfault */
		freebytes(data_to_free->key, data_to_free->key_len * sizeof(char));
		freebytes(data_to_free->value, data_to_free->value_len * sizeof(char));
		freebytes(data_to_free, sizeof(struct _key_value_pair));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->first_data = NULL;
	x->last_data = NULL;
}
