void setup_rest(void) {
	rest_class = class_new(gensym("rest-json"), (t_newmethod)rest_new,
			0, sizeof(t_rest), 0, A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_oauth, gensym("oauth"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_url, gensym("url"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("PUT"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("DELETE"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("POST"), A_GIMME, 0);
	class_sethelpsymbol(rest_class, gensym("rest-json"));
}

void rest_command(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *request_type;
	char path[MAX_STRING_SIZE];
	char parameter[MAX_STRING_SIZE];
	switch (argcount) {
		case 0:
			break;
		default:
			request_type = selector->s_name;
			atom_string(argvec, path, MAX_STRING_SIZE);
			if (argcount > 1) {
				atom_string(argvec + 1, parameter, MAX_STRING_SIZE);
			}
			execute_rest(x->base_url, request_type, path, parameter, x);
			break;
	}
}

void rest_oauth(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {
	error("OAUTH not implemented yet.");
}

void rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {
	switch (argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->base_url, MAX_STRING_SIZE);
			}
			break;
		case 0:
			break;
		default:
			error("Too many parameters.");
			break;
	}
}

void *rest_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_rest *x = (t_rest *)pd_new(rest_class);
	switch (argcount) {
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->base_url, MAX_STRING_SIZE);
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
	x->is_data_locked = 0;
	return (void *)x;
}

void *execute_rest_thread(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	char parameters[strlen(x->parameters) + 1];
	char request_url[strlen(x->complete_url) + 1];;
	char request_type[strlen(x->request_type) + 1];
	CURL *curl_handle;
	CURLcode result;
	t_memory_struct in_memory;
	t_memory_struct out_memory;
	strcpy(parameters, x->parameters);
	strcpy(request_type, x->request_type);
	strcpy(request_url, x->complete_url);
	x->is_data_locked = 0;
	size_t parameter_len = strlen(parameters);
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, request_url);
		if (strcmp(request_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_memory_callback);
			/* Prepare data for reading */
			in_memory.memory = malloc(parameter_len + 1);
			in_memory.size = parameter_len;
			memcpy(in_memory.memory, parameters, parameter_len);
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
			if (!is_error(jobj)) {
				json_object_put(jobj);
			}
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

	pthread_exit(NULL);
}

void execute_rest(char *base_url, char *request_type, char *path, char *parameters, t_rest *x) {
	char *cleaned_parameters = remove_backslashes(parameters);
	int rc;
	pthread_t thread;
	pthread_attr_t thread_attributes;
	strcpy(x->complete_url, base_url);
	strcat(x->complete_url, path);
	strcpy(x->request_type, request_type);
	strcpy(x->parameters, cleaned_parameters);
	free(cleaned_parameters);
	pthread_attr_init(&thread_attributes);
	pthread_attr_setstacksize(&thread_attributes, sizeof(double) * 1024 + 1000000);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	if (x->is_data_locked) {
		error("data of object locked");
		return;
	}
	x->is_data_locked = 1;
	rc = pthread_create(&thread, &thread_attributes, execute_rest_thread, (void *)x);
	pthread_attr_destroy(&thread_attributes);
	if (rc) {
		error("Could not create thread with code %d", rc);
		x->is_data_locked = 0;
	}
}

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	t_memory_struct *mem = (t_memory_struct *)data;
	mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */ 
		error("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}
	memcpy(&mem->memory[mem->size], ptr, realsize);
	if (mem->size + realsize - mem->size != realsize) {
		error("Integer overflow or similar. Bad Things can happen.");
	}
	mem->size += realsize;
	mem->memory[mem->size] = '\0';

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

