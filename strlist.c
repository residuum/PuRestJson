struct _strlist {
	char *str;
	size_t str_len;
	struct _strlist *next;
};

static struct _strlist *strlist_create(char *val, size_t val_len);
static struct _strlist *strlist_add(struct _strlist *list, char *val, size_t val_len);
static void strlist_free(struct _strlist *list);

/* begin implementations */
static struct _strlist *strlist_create(char *val, size_t val_len) {
	struct _strlist *created_data = NULL;

	created_data = getbytes(sizeof(struct _strlist));
	created_data->str = string_create(&created_data->str_len, val_len);
	if (created_data == NULL || val == NULL) {
		MYERROR("Could not get data");
		return NULL;
	}

	strcpy(created_data->str, val);
	created_data->next = NULL;

	return created_data;
}

static struct _strlist *strlist_add(struct _strlist *list, char *val, size_t val_len) {
	struct _strlist *new = strlist_create(val, val_len);
	struct _strlist *existing = list;
	
	if (list == NULL) {
		return new;
	}
	while (existing != NULL) {
		if (existing->next != NULL) {
			existing = existing->next;
		} else {
			existing->next = new;
			break;
		}
	}
	return list;
}

static void strlist_free(struct _strlist *list) {
	struct _strlist *data_to_free = list;
	struct _strlist *next_data;

	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		string_free(data_to_free->str, &data_to_free->str_len);
		freebytes(data_to_free, sizeof(struct _strlist));
		data_to_free = next_data;
	}
}
