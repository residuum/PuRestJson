#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <pthread.h>
#include <oauth.h>

#define LIBRARY_VERSION "0.8"

#define REQUEST_TYPE_LEN 7

/* reading / writing data in HTTP requests */
typedef struct memory_struct {
	char *memory;
	size_t size;
} t_memory_struct;

/* storing data before encoding to JSON */
typedef struct key_value_pair {
	char *key;
	char *value;
	short is_array;
	struct key_value_pair *next;
} t_key_value_pair;

/* data for threading */
typedef struct rest_common {
	t_object x_ob;
	t_outlet *status_info_outlet;
	char request_type[REQUEST_TYPE_LEN]; /*One of GET, PUT, POST; DELETE*/
	char parameters[MAXPDSTRING];
	char complete_url[MAXPDSTRING];
	short is_data_locked;
	char base_url[MAXPDSTRING];
	t_atom *out;
} t_rest_common;

/* [rest] */
typedef struct rest {
	t_rest_common threaddata;
	/* authentication: cookie */
	struct {
		char login_path[MAXPDSTRING];
		char username[MAXPDSTRING];
		char password[MAXPDSTRING];
		char auth_token[MAXPDSTRING];
	} cookie;
} t_rest;

/* [oauth] */
typedef struct oauth {
	t_rest_common threaddata;
	/* authentication*/
	struct {
		char client_key[MAXPDSTRING];
		char client_secret[MAXPDSTRING];
		char token_key[MAXPDSTRING];
		char token_secret[MAXPDSTRING];
		OAuthMethod method;
	} oauth;
} t_oauth;

typedef struct kvp_storage {
	t_object x_ob;
	t_key_value_pair *first_data;
	t_key_value_pair *last_data;
	int data_count;
} t_kvp_storage;

/* [json-encode] */
typedef struct json_encode {
	t_kvp_storage storage;
	t_canvas *x_canvas;
} t_json_encode;

/* [json-decode] */
typedef struct json_decode {
	t_object x_ob;
	t_outlet *done_outlet;
} t_json_decode;

/* [urlparams] */
typedef struct urlparams {
	t_kvp_storage storage;
} t_urlparams;

/* [rest] */
void rest_setup(void);
void *rest_new(t_symbol *selector, int argcount, t_atom *argvec);

void rest_command(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec); 
void rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [oauth] */
void oauth_setup(void);
void *oauth_new(t_symbol *selector, int argcount, t_atom *argvec);

void oauth_command(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec); 
void oauth_url(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec);
void oauth_method(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [json-encode] */
void setup_json0x2dencode(void);
void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec);
void json_encode_free(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);

void json_encode_bang(t_json_encode *x);
void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);
void json_encode_array_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);
void json_encode_read(t_json_encode *x, t_symbol *filename);
void json_encode_write(t_json_encode *x, t_symbol *filename);
void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [json-decode] */
void setup_json0x2ddecode(void);
void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec);

void json_decode_string(t_json_decode *x, t_symbol *data);
void json_decode_list(t_json_decode *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [urlparams] */
void urlparams_setup(void);
void *urlparams_new(t_symbol *selector, int argcount, t_atom *argvec);
void urlparams_free(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec);

void urlparams_bang(t_urlparams *x);
void urlparams_add(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec);
void urlparams_clear(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec);

/* general */ 
void purest_json_setup(void);
char *remove_backslashes(char *source_string, size_t memsize);


char *remove_backslashes(char *source_string, size_t memsize) {
	char *cleaned_string = NULL;
	char *masking = "\\";
	char *segment;
	size_t len_src = strlen(source_string);

	memsize = (len_src + 1) * sizeof(char);

	cleaned_string = (char *) getbytes(memsize * sizeof(char));
	if (cleaned_string == NULL) {
		error("Unable to allocate memory\n");
	} else if (len_src > 0) {
		segment = strtok(source_string, masking);
		strcpy(cleaned_string, segment);
		segment = strtok(NULL, masking);
		while (segment != NULL) {
			if (segment[0] != ',') {
				/* We keep the backslash */
				strcat(cleaned_string, masking);
			}
			strcat(cleaned_string, segment);
			segment = strtok(NULL, masking);
		}
	}
	return (cleaned_string);
}
