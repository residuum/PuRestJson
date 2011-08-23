void couchdb_command(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char request_type[7];
	char data[MAX_STRING_SIZE];
	char *additional_parameters[16];
	int i;
	switch (argcount) {
		case 0:
			break;
		default:
			atom_string(argvec, request_type, 7);
			if (argcount > 1) {
				atom_string(argvec + 1, data, MAX_STRING_SIZE);
				for (i = 2; i < argcount; i++) {
					atom_string(argvec + i, additional_parameters[i - 1], MAX_STRING_SIZE);
				} 
			}
			execute_couchdb(x->couch_url, request_type, data, additional_parameters);
			break;
	}
}

void couchdb_oauth(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	error("OAUTH not implemented yet.");
}

void couchdb_url(t_couchdb *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char url[MAX_STRING_SIZE];
	switch (argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("URL to CouchDB cannot be set.");
			} else {
				atom_string(argvec, url, MAX_STRING_SIZE);
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
	char url[MAX_STRING_SIZE];
	switch (argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("URL to CouchDB cannot be set.");
			} else {
				atom_string(argvec, url, MAX_STRING_SIZE);
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

void test_connection(char *couch_url) {
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

void execute_couchdb(char *couch_url, char *request_type, char *data, char **additional_parameters){
	char real_url[strlen(couch_url) + strlen(data)];
	CURL *curl_handle;
	CURLcode result;
	strcpy(real_url, couch_url);
	strcat(real_url, data);
	t_memory_struct chunk;
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, real_url);
		post("Request URL: %s", real_url);
		if (strcmp(request_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
		} else if (strcmp(request_type, "POST") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
		} else if (strcmp(request_type, "DELETE") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}
		chunk.memory = malloc(1);
		chunk.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
		result = curl_easy_perform(curl_handle);
		if (result == CURLE_OK) {
			post("Action successful");
			post("Received information: %s", chunk.memory);
			if (chunk.memory) {
				free(chunk.memory);
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
