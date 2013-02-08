struct _memory_struct {
	char *memory;
	size_t size;
};

struct _rest_common {
	t_object x_ob;
	t_outlet *status_info_outlet;
	char request_type[REQUEST_TYPE_LEN]; /*One of GET, PUT, POST; DELETE*/
	char parameters[MAXPDSTRING];
	char complete_url[MAXPDSTRING];
	short is_data_locked;
	char base_url[MAXPDSTRING];
	long timeout;
	t_atom *out;
};

struct _rest {
	struct _rest_common threaddata;
	/* authentication: cookie */
	struct {
		char login_path[MAXPDSTRING];
		char username[MAXPDSTRING];
		char password[MAXPDSTRING];
		char auth_token[MAXPDSTRING];
	} cookie;
};

struct _oauth {
	struct _rest_common threaddata;
	/* authentication */
	struct {
		char client_key[MAXPDSTRING];
		char client_secret[MAXPDSTRING];
		char token_key[MAXPDSTRING];
		char token_secret[MAXPDSTRING];
		OAuthMethod method;
		char *rsa_key;
	} oauth;
};

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct _memory_struct *mem = (struct _memory_struct *)data;

	mem->memory = (char *) resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
	if (mem->memory == NULL) {
		/* out of memory! */ 
		error("not enough memory");
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
	struct _memory_struct *mem = (struct _memory_struct *)data;
	size_t to_copy = (mem->size < realsize) ? mem->size : realsize;

	memcpy(ptr, mem->memory, to_copy);
	mem->size -= to_copy;
	mem->memory += to_copy;
	return to_copy;
}

static void *execute_request(void *thread_args) {
	struct _rest_common *threaddata = (struct _rest_common *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	struct _memory_struct in_memory;
	struct _memory_struct out_memory;
	long http_status;
	t_atom http_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, threaddata->complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, threaddata->timeout);
		t_rest *x = (t_rest *)threaddata;
		if (strlen(x->cookie.auth_token) != 0) {
			curl_easy_setopt(curl_handle, CURLOPT_COOKIE, x->cookie.auth_token);
		}
		if (strcmp(threaddata->request_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_memory_callback);
			/* Prepare data for reading */
			in_memory.memory = getbytes(strlen(threaddata->parameters) + 1);
			if (in_memory.memory == NULL) {
				error("not enough memory");
			}
			in_memory.size = strlen(threaddata->parameters);
			memcpy(in_memory.memory, threaddata->parameters, strlen(threaddata->parameters));
			curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&in_memory);
		} else if (strcmp(threaddata->request_type, "POST") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, threaddata->parameters);
		} else if (strcmp(threaddata->request_type, "DELETE") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}
		out_memory.memory = getbytes(1);
		out_memory.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_memory);
		result = curl_easy_perform(curl_handle);

		/* output status */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_status);
		SETSYMBOL(&http_status_data[0], gensym(threaddata->request_type));
		if (http_status >= 200 && http_status < 300) {
			SETSYMBOL(&http_status_data[1], gensym("bang"));
			outlet_list(threaddata->status_info_outlet, &s_list, 2, &http_status_data[0]);
			if (result == CURLE_OK) {
				outlet_symbol(threaddata->x_ob.ob_outlet, gensym(out_memory.memory));
				/* Free memory */
				if (out_memory.memory) {
					freebytes(out_memory.memory, (out_memory.size + 1) * sizeof(char));
				}
				free((void *)result);
			} else {
				SETFLOAT(&http_status_data[1], (float)http_status);
				SETFLOAT(&http_status_data[2], (float)result);
				outlet_list(threaddata->status_info_outlet, &s_list, 3, &http_status_data[0]);
				error("Error while performing request: %s", curl_easy_strerror(result));
			}
		} else {
			SETFLOAT(&http_status_data[1], (float)http_status);
			SETFLOAT(&http_status_data[2], (float)result);
			error("HTTP error while performing request: %li", http_status);
			outlet_list(threaddata->status_info_outlet, &s_list, 3, &http_status_data[0]);
		}
		curl_easy_cleanup(curl_handle);
	} else {
		error("Cannot init curl.");
	}
	threaddata->is_data_locked = 0;
	return NULL;
}

static void thread_execute(struct _rest_common *x, void *(*func) (void *)) {
	int rc;
	pthread_t thread;
	pthread_attr_t thread_attributes;

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&thread, &thread_attributes, func, (void *)x);
	pthread_attr_destroy(&thread_attributes);
	if (rc) {
		error("Could not create thread with code %d", rc);
		x->is_data_locked = 0;
	}
}

static void set_timeout(struct _rest_common *x, int timeout) {
	x->timeout = (long) timeout;
}
