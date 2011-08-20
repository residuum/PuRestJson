#include "couchdb.h"

void couchdb_command(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	int i;
	for (i = 0; i < argcount; i++) {
		if (argvec[i].a_type == A_FLOAT)
			post("float: %f", argvec[i].a_w.w_float);
		else if (argvec[i].a_type == A_SYMBOL)
			post("symbol: %s", argvec[i].a_w.w_symbol->s_name);
	}
}

void couchdb_oauth(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	error("OAUTH not implemented yet.");
}

void couchdb_url(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char url[128] = "http://127.0.0.1:5984/";
	switch(argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("URL to CouchDB cannot be set.");
			} else {
				atom_string(argvec, url, 128);
				x->couch_url = (char *)url;
				test_connection(x->couch_url);
			}
			break;
		case 0:
			break;
		default:
			error("Too many parameters.");
			break;
	}
}

void *couchdb_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_couchdb *x = (t_couchdb *)pd_new(couchdb_class);
	char url[128] = "http://127.0.0.1:5984/";
	switch(argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("URL to CouchDB cannot be set.");
			} else {
				atom_string(argvec, url, 128);
				x->couch_url = (char *)url;
				test_connection(x->couch_url);
			}
			break;
		case 0:
			break;
		default:
			error("Too many parameters.");
			break;
	}
	return (void *)x;
}

void couchdb_setup(void) {
	couchdb_class = class_new(gensym("couchdb"), (t_newmethod)couchdb_new,
			0, sizeof(t_couchdb), 0, A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_oauth, gensym("oauth"), A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_url, gensym("url"), A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_command, gensym("couchdb"), A_GIMME, 0);
}

void test_connection(char *couch_url) {
	CURL *curl_handle;
	CURLcode result;
	t_memory_struct chunk;
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		/*curl_easy_setopt(curl_handle, CURLOPT_URL, couch_url);*/
		curl_easy_setopt(curl_handle, CURLOPT_URL, couch_url);
		chunk.memory = malloc(1);
		chunk.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
		result = curl_easy_perform(curl_handle);
		if (result == CURLE_OK) {
			post("Connection to %s established.", couch_url);
			post("Received information: %s", chunk.memory);
			if (chunk.memory) {
				free(chunk.memory);
			}
		} else {
			error("Could not establish connection to %s with error %s.", couch_url, curl_easy_strerror(result));
		}
		curl_easy_cleanup(curl_handle);
	} else {
		error("Cannot init curl.");
	}
}

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	t_memory_struct *mem = (t_memory_struct *)data;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */ 
		error("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
