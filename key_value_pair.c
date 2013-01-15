static t_key_value_pair *create_key_value_pair(char *key, char *value, int is_array){
	t_key_value_pair *created_data = NULL;

	created_data = (t_key_value_pair *)getbytes(sizeof(t_key_value_pair));
	created_data->key = (char *)getbytes(MAXPDSTRING * sizeof(char));
	created_data->value = (char *)getbytes(MAXPDSTRING * sizeof(char));
	if (created_data == NULL || key == NULL || value == NULL) {
		error("Could not get data");
		return NULL;
	}
	strcpy(created_data->key, key);
	strcpy(created_data->value, value);
	created_data->next = NULL;
	created_data->is_array = is_array;

	return created_data;
}

static void kvp_storage_add(t_kvp_storage *x, t_key_value_pair *new_pair) {
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

static void kvp_storage_free_memory(t_kvp_storage *x) {
	t_key_value_pair *data_to_free;
	t_key_value_pair *next_data;

	data_to_free = x->first_data;
	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		/* TODO: Investigate the reason for segfault */
		freebytes(data_to_free->key, MAXPDSTRING);
		freebytes(data_to_free->value, MAXPDSTRING);
		freebytes(data_to_free, sizeof(t_key_value_pair));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->first_data = NULL;
	x->last_data = NULL;
}
