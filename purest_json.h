#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <pthread.h>

#define LIBRARY_VERSION "0.7"

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

/* [rest-json] */
typedef struct rest {
	t_object x_ob;
	t_outlet *done_outlet;
	t_outlet *status_info_outlet;
	int out_count;
	char base_url[MAXPDSTRING];
	/* cookie authentication */
	char login_path[MAXPDSTRING];
	char username[MAXPDSTRING];
	char password[MAXPDSTRING];
	char auth_token[MAXPDSTRING];
	/* end cookie authentication */
	t_atom *out;
	/* threading */
	char request_type[7]; /*One of GET, PUT, POST; DELETE*/
	char parameters[MAXPDSTRING];
	char complete_url[MAXPDSTRING];
	short is_data_locked;
	/* end threading */
} t_rest;

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
 
/* [rest-json] */
void setup_rest0x2djson(void);
void *rest_new(t_symbol *selector, int argcount, t_atom *argvec);

void rest_command(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec); 
void rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);
void rest_oauth(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);

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

/* general */ 
void purest_json_setup(void);
char *remove_backslashes(char *source_string, size_t memsize);
int str_ccmp(const char *s1, const char *s2);
