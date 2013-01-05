static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	t_memory_struct *mem = (t_memory_struct *)data;

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
	t_memory_struct *mem = (t_memory_struct *)data;
	size_t to_copy = (mem->size < realsize) ? mem->size : realsize;

	memcpy(ptr, mem->memory, to_copy);
	mem->size -= to_copy;
	mem->memory += to_copy;
	return to_copy;
}

static void *execute_request(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	t_memory_struct in_memory;
	t_memory_struct out_memory;
	long http_code;
	t_atom http_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, x->threaddata.complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		if (strlen(x->cookie.auth_token) != 0) {
			curl_easy_setopt(curl_handle, CURLOPT_COOKIE, x->cookie.auth_token);
		}
		if (strcmp(x->threaddata.request_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_memory_callback);
			/* Prepare data for reading */
			in_memory.memory = getbytes(strlen(x->threaddata.parameters) + 1);
			if (in_memory.memory == NULL) {
				error("not enough memory");
			}
			in_memory.size = strlen(x->threaddata.parameters);
			memcpy(in_memory.memory, x->threaddata.parameters, strlen(x->threaddata.parameters));
			curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&in_memory);
		} else if (strcmp(x->threaddata.request_type, "POST") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, x->threaddata.parameters);
		} else if (strcmp(x->threaddata.request_type, "DELETE") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}
		out_memory.memory = getbytes(1);
		out_memory.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_memory);
		result = curl_easy_perform(curl_handle);

		/* output status */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
		SETSYMBOL(&http_status_data[0], gensym(x->threaddata.request_type));
		if (http_code == 200) {
			SETSYMBOL(&http_status_data[1], gensym("bang"));
			outlet_list(x->status_info_outlet, &s_list, 2, &http_status_data[0]);
			if (result == CURLE_OK) {
				outlet_symbol(x->x_ob.ob_outlet, gensym(out_memory.memory));
				/* Free memory */
				if (out_memory.memory) {
					freebytes(out_memory.memory, (out_memory.size + 1) * sizeof(char));
				}
				free((void *)result);
			} else {
				SETFLOAT(&http_status_data[1], (float)http_code);
				SETFLOAT(&http_status_data[2], (float)result);
				outlet_list(x->status_info_outlet, &s_list, 3, &http_status_data[0]);
				error("Error while performing request: %s", curl_easy_strerror(result));
			}
		} else {
			SETFLOAT(&http_status_data[1], (float)http_code);
			SETFLOAT(&http_status_data[2], (float)result);
			error("Error while performing request: %s", curl_easy_strerror(result));
			outlet_list(x->status_info_outlet, &s_list, 3, &http_status_data[0]);
		}
		curl_easy_cleanup(curl_handle);
	} else {
		error("Cannot init curl.");
	}
	x->threaddata.is_data_locked = 0;
	return NULL;
}
