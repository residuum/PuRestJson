/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2022 Thomas Mayer

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

struct _strlist {
	char *str;
	size_t str_len;
	struct _strlist *next; /* linked list */
};

/* creates new item */
static struct _strlist *strlist_create(char *val, const  size_t val_len);
/* adds item to list */
static struct _strlist *strlist_add(struct _strlist *list, char *val, const size_t val_len);
/* frees list */
static void strlist_free(struct _strlist *list);

/* begin implementations */
static struct _strlist *strlist_create(char *const val, const size_t val_len) {
	struct _strlist *created_data = NULL;

	created_data = getbytes(sizeof(struct _strlist));
	if (created_data == NULL || val == NULL) {
		pd_error(0, "Could not get data.");
		return NULL;
	}

	created_data->str = string_create(&created_data->str_len, val_len);
	strcpy(created_data->str, val);
	created_data->next = NULL;

	return created_data;
}

static struct _strlist *strlist_add(struct _strlist *const list, char *const val, const size_t val_len) {
	struct _strlist *const new = strlist_create(val, val_len);
	struct _strlist *it = list;

	if (list == NULL) {
		return new;
	}
	while (it != NULL) {
		if (it->next != NULL) {
			it = it->next;
		} else {
			it->next = new;
			break;
		}
	}
	return list;
}

static void strlist_free(struct _strlist *list) {
	struct _strlist *data_to_free = list;

	while(data_to_free != NULL) {
		struct _strlist *next_data = data_to_free->next;
		string_free(data_to_free->str, &data_to_free->str_len);
		freebytes(data_to_free, sizeof(struct _strlist));
		data_to_free = next_data;
	}
}
