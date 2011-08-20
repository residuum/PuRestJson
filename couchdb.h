#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>

#define MAX_ARRAY_SIZE 512

typedef struct couchdb {
	t_object x_ob;
	int out_count;
	char *couch_url;
	t_atom out[MAX_ARRAY_SIZE];
} t_couchdb;

 
typedef struct memory_struct {
  char *memory;
  size_t size;
} t_memory_struct;

void couchdb_command(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec); 
void couchdb_oauth(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec);
void couchdb_url(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec);

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data);
void test_connection(char *couch_url);

t_class *couchdb_class;
void *couchdb_new(t_symbol *selector, int argcount, t_atom *argvec);
void couchdb_setup(void); 
