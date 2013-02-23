struct _memory_struct {
	char *memory;
	size_t size;
};

struct _rest_common {
	t_object x_ob;
	t_outlet *stat_out;
	char req_type[REQUEST_TYPE_LEN]; /*One of GET, PUT, POST; DELETE*/
	size_t base_url_len;
	char *base_url;
	size_t parameters_len;
	char *parameters;
	size_t complete_url_len;
	char *complete_url;
	short locked;
	long timeout;
	t_atom *out;
	short is_rest;
};

struct _rest {
	struct _rest_common common;
	/* authentication: cookie */
	struct {
		size_t login_path_len;
		char *login_path;
		size_t username_len;
		char *username;
		size_t password_len;
		char *password;
		size_t auth_token_len;
		char *auth_token;
	} cookie;
};

struct _oauth {
	struct _rest_common common;
	/* authentication */
	struct {
		size_t client_key_len;
		char *client_key;
		size_t client_secret_len;
		char *client_secret;
		size_t token_key_len;
		char *token_key;
		size_t token_secret_len;
		char *token_secret;
		OAuthMethod method;
		size_t rsa_key_len;
		char *rsa_key;
	} oauth;
};

static char *get_string(size_t *newl, size_t strl) {
	char *gen;
	(*newl) = 1 + strl;
	gen = (char *)getbytes((*newl) * sizeof(char));
	return memset(gen, 0x00, (*newl));
}

static void free_string(char *string, size_t *strl) {
	if ((*strl)) {
		freebytes(string, (*strl) * sizeof(char));
		(*strl) = 0;
	}
}

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct _memory_struct *mem = (struct _memory_struct *)data;

	mem->memory = (char *) resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
	if (mem->memory == NULL) {
		/* out of memory! */ 
		myerror("not enough memory");
	}
	memcpy(&mem->memory[mem->size], ptr, realsize);
	if (mem->size + realsize - mem->size != realsize) {
		myerror("Integer overflow or similar. Bad Things can happen.");
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
	struct _rest_common *common = (struct _rest_common *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	struct _memory_struct in_memory;
	struct _memory_struct out_memory;
	long http_status;
	t_atom http_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, common->complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, common->timeout);
		if (common->is_rest) {
			t_rest *x = (t_rest *)common;
			if (x->cookie.auth_token_len) {
				curl_easy_setopt(curl_handle, CURLOPT_COOKIE, x->cookie.auth_token);
			}
		}
		if (strcmp(common->req_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_memory_callback);
			/* Prepare data for reading */
			if (common->parameters_len) {
				in_memory.memory = getbytes(strlen(common->parameters) + 1);
				in_memory.size = strlen(common->parameters);
				if (in_memory.memory == NULL) {
					myerror("not enough memory");
				}
				memcpy(in_memory.memory, common->parameters, common->parameters_len - 1);
			} else {
				in_memory.memory = NULL;
				in_memory.size = 0;
			}
			curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&in_memory);
		} else if (strcmp(common->req_type, "POST") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, common->parameters);
		} else if (strcmp(common->req_type, "DELETE") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}
		out_memory.memory = getbytes(1);
		out_memory.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_memory);
		result = curl_easy_perform(curl_handle);

		/* output status */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_status);
		SETSYMBOL(&http_status_data[0], gensym(common->req_type));
		if (http_status >= 200 && http_status < 300) {
			SETSYMBOL(&http_status_data[1], gensym("bang"));
			outlet_list(common->stat_out, &s_list, 2, &http_status_data[0]);
			if (result == CURLE_OK) {
				outlet_symbol(common->x_ob.ob_outlet, gensym(out_memory.memory));
				/* Free memory */
				free_string(out_memory.memory, &out_memory.size);
				free((void *)result);
			} else {
				SETFLOAT(&http_status_data[1], (float)http_status);
				SETFLOAT(&http_status_data[2], (float)result);
				outlet_list(common->stat_out, &s_list, 3, &http_status_data[0]);
				myerror("Error while performing request: %s", curl_easy_strerror(result));
			}
		} else {
			SETFLOAT(&http_status_data[1], (float)http_status);
			SETFLOAT(&http_status_data[2], (float)result);
			myerror("HTTP error while performing request: %li", http_status);
			outlet_list(common->stat_out, &s_list, 3, &http_status_data[0]);
		}
		curl_easy_cleanup(curl_handle);
	} else {
		myerror("Cannot init curl.");
	}
	free_string(common->complete_url, &common->complete_url_len);
	free_string(common->parameters, &common->parameters_len);
	common->locked = 0;
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
		myerror("Could not create thread with code %d", rc);
		free_string(x->complete_url, &x->complete_url_len);
		free_string(x->parameters, &x->parameters_len);
		x->locked = 0;
	}
}

static void set_timeout(struct _rest_common *x, int timeout) {
	x->timeout = (long) timeout;
}

static void rest_common_free(struct _rest_common *x) {
	free_string(x->base_url, &x->base_url_len);
	free_string(x->parameters, &x->parameters_len);
	free_string(x->complete_url, &x->complete_url_len);
}
