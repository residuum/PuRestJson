#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <pthread.h>
#include <oauth.h>

#define LIBRARY_VERSION "0.7.1"

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

/* [rest] */
typedef struct rest {
	t_object x_ob;
	t_outlet *status_info_outlet;
	char base_url[MAXPDSTRING];
	/* authentication: cookie */
	struct {
		char login_path[MAXPDSTRING];
		char username[MAXPDSTRING];
		char password[MAXPDSTRING];
		char auth_token[MAXPDSTRING];
	} cookie;
	t_atom *out;
	/* threading */
	char request_type[7]; /*One of GET, PUT, POST; DELETE*/
	char parameters[MAXPDSTRING];
	char complete_url[MAXPDSTRING];
	short is_data_locked;
	/* end threading */
} t_rest;

/* [oauth] */
typedef struct oauth {
	t_object x_ob;
	t_outlet *status_info_outlet;
	char base_url[MAXPDSTRING];
	/* authentication*/
	struct {
		char client_key[MAXPDSTRING];
		char client_secret[MAXPDSTRING];
		char token_key[MAXPDSTRING];
		char token_secret[MAXPDSTRING];
		OAuthMethod method;
	} oauth;
	t_atom *out;
	/* threading */
	char request_type[5]; /*GET or POST*/
	char parameters[MAXPDSTRING];
	char complete_url[MAXPDSTRING];
	short is_data_locked;
	/* end threading */
} t_oauth;

/* [json-encode] */
typedef struct json_encode {
	t_object x_ob;
	t_key_value_pair *first_data;
	t_key_value_pair *last_data;
	int data_count;
} t_json_encode;

/* [json-decode] */
typedef struct json_decode {
	t_object x_ob;
	t_outlet *done_outlet;
} t_json_decode;

/* [urlparams] */
typedef struct urlparams {
	t_object x_ob;
	t_key_value_pair *first_data;
	t_key_value_pair *last_data;
	int data_count;
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
void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [json-decode] */
void setup_json0x2ddecode(void);
void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec);

void json_decode_string(t_json_decode *x, t_symbol *data);
void json_decode_list(t_json_decode *x, t_symbol *selector, int argcount, t_atom *argvec);
void output_json(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet);
void output_json_string(char *json_string, t_outlet *data_outlet, t_outlet *done_outlet);

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
int str_ccmp(const char *s1, const char *s2);
