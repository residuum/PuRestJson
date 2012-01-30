#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <pthread.h>

#define LIBRARY_VERSION "0.5"

#define MAX_ARRAY_SIZE 256
#define MAX_STRING_SIZE 2048 

typedef struct memory_struct {
  char *memory;
  size_t size;
} t_memory_struct;

typedef struct key_value_pair {
  char key[MAX_STRING_SIZE];
  char value[MAX_STRING_SIZE];
  short is_array;
} t_key_value_pair;

typedef struct rest {
	t_object x_ob;
	t_outlet *done_outlet;
	int out_count;
	char base_url[MAX_STRING_SIZE];
	t_atom out[MAX_ARRAY_SIZE];
	/* Used for threading */
	char request_type[6]; /*One of GET, PUT, POST; DELETE*/
	char parameters[MAX_STRING_SIZE];
	char complete_url[MAX_STRING_SIZE];
	short is_data_locked;
} t_rest;

typedef struct json_encode {
	t_object x_ob;
	t_key_value_pair data[MAX_ARRAY_SIZE];
	int data_count;
} t_json_encode;

typedef struct json_decode {
	t_object x_ob;
	t_outlet *done_outlet;
} t_json_decode;
 
/* rest */
void setup_rest0x2djson(void);
void *rest_new(t_symbol *selector, int argcount, t_atom *argvec);

void rest_command(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec); 
void rest_oauth(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);
void rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);

void execute_rest(t_rest *x);

/* pthread functions */
void *execute_rest_thread(void *thread_args);

/* json-encode */
void setup_json0x2dencode(void);
void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec);

void json_encode_bang(t_json_encode *x);
void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);
void json_encode_array_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);
void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);

json_object *create_object(char *value);

/* json-decode */
void setup_json0x2ddecode(void);
void *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec);

void json_decode_string(t_json_decode *x, t_symbol *data);
void json_decode_list(t_json_decode *x, t_symbol *selector, int argcount, t_atom *argvec);
void output_json(json_object *jobj, t_outlet *data_outlet, t_outlet *done_outlet);

/* general */ 
void purest_json_setup(void);
char *remove_backslashes(char *source_string);
int str_ccmp(const char *s1, const char *s2);
