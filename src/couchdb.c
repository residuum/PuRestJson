/**
 * Setup couchdb 
 * 
 * Performs setup of couchdb object, initializes methods for inlet
 */
void setup_couchdb(void) {
	couchdb_class = class_new(gensym("couchdb"), (t_newmethod)couchdb_new,
			0, sizeof(t_couchdb), 0, A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_oauth, gensym("oauth"), A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_url, gensym("url"), A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_command, gensym("couchdb"), A_GIMME, 0);
}

/**
 * Performs couchdb method 
 * 
 * Executes a couchdb command 
 */
void couchdb_command(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char request_type[8];
	char database[MAX_STRING_SIZE];
	char parameter[MAX_STRING_SIZE];
	switch (argcount) {
		case 0:
			break;
		default:
			atom_string(argvec, request_type, 8);
			if (argcount > 1) {
				atom_string(argvec + 1, database, MAX_STRING_SIZE);
			} 
			if (argcount > 2) {
				atom_string(argvec + 2, parameter, MAX_STRING_SIZE);
			}
			execute_couchdb(x->couch_url, request_type, database, parameter, x);
			break;
	}
}

void couchdb_oauth(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	error("OAUTH not implemented yet.");
}

void couchdb_url(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	switch (argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("URL to CouchDB cannot be set.");
			} else {
				atom_string(argvec, x->couch_url, MAX_STRING_SIZE);
				test_connection(x->couch_url, x);
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
	switch (argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("URL to CouchDB cannot be set.");
			} else {
				atom_string(argvec, x->couch_url, MAX_STRING_SIZE);
				test_connection(x->couch_url, x);
			}
			break;
		case 0:
			break;
		default:
			error("Too many parameters.");
			break;
	}
	outlet_new(&x->x_ob, NULL);
	x->done_outlet = outlet_new(&x->x_ob, &s_bang);
	return (void *)x;
}

void test_connection(char *couch_url, t_couchdb *x) {
	CURL *curl_handle;
	CURLcode result;
	t_memory_struct chunk;
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, couch_url);
		chunk.memory = malloc(1);
		chunk.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
		result = curl_easy_perform(curl_handle);
		if (result == CURLE_OK) {
			post("Connection to %s possible.", couch_url);
			post("Received information: %s", chunk.memory);
			/* Free memory */
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

void execute_couchdb(char *couch_url, char *request_type, char *database, char *parameters, t_couchdb *x){
	char real_url[strlen(couch_url) + strlen(database)];
	char *cleaned_parameters = remove_backslashes(parameters);
	CURL *curl_handle;
	CURLcode result;
	strcpy(real_url, couch_url);
	strcat(real_url, database);
	t_memory_struct in_memory;
	t_memory_struct out_memory;
	size_t parameter_len = strlen(cleaned_parameters);
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, real_url);
		if (strcmp(request_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_memory_callback);
			/* Prepare data for reading */
			in_memory.memory = malloc(parameter_len + 1);
			in_memory.size = parameter_len;
			memcpy(in_memory.memory, cleaned_parameters, parameter_len);
			curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&in_memory);
		} else if (strcmp(request_type, "POST") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
		} else if (strcmp(request_type, "DELETE") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}
		out_memory.memory = malloc(1);
		out_memory.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_memory);
		result = curl_easy_perform(curl_handle);
		if (result == CURLE_OK) {
			/* Parse JSON */
			json_object *jobj = json_tokener_parse(out_memory.memory);
			output_json(jobj, x->x_ob.ob_outlet, x->done_outlet);
			/* Free memory */
			if (out_memory.memory) {
				free(out_memory.memory);
			}
		} else {
			error("Error while performing request: %s", curl_easy_strerror(result));
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

static size_t read_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	t_memory_struct *mem = (t_memory_struct *)data;
	size_t to_copy = (mem->size < realsize) ? mem->size : realsize;
	memcpy(ptr, mem->memory, to_copy);
	mem->size -= to_copy;
	mem->memory += to_copy;
	return to_copy;
}

