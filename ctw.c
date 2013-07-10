#ifdef NEEDS_CERT_PATH
#include "m_imp.h"
#endif
struct _memory_struct {
	char *memory;
	size_t size;
};

struct _ctw {
	t_object x_ob;
	t_outlet *stat_out;
	t_atom *out;
	pthread_t thread;
	struct _strlist *http_headers;
	char req_type[REQUEST_TYPE_LEN]; /*One of GET, PUT, POST; DELETE*/
	size_t base_url_len;
	char *base_url;
	size_t parameters_len;
	char *parameters;
	size_t complete_url_len;
	char *complete_url;
	size_t auth_token_len;
	char *auth_token;
	unsigned char locked;
	long timeout;
	unsigned char sslcheck;
#ifdef NEEDS_CERT_PATH
	size_t cert_path_len;
	char *cert_path;
#endif
};

static size_t ctw_write_mem_cb(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct _memory_struct *mem = data;

	mem->memory = resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
	if (mem->memory == NULL) {
		MYERROR("not enough memory");
	}
	memcpy(&mem->memory[mem->size], ptr, realsize);
	if (mem->size + realsize - mem->size != realsize) {
		MYERROR("Integer overflow or similar. Bad Things can happen.");
	}
	mem->size += realsize;
	mem->memory[mem->size] = '\0';
	return realsize;
}

static size_t ctw_read_mem_cb(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct _memory_struct *mem = data;
	size_t to_copy = (mem->size < realsize) ? mem->size : realsize;

	memcpy(ptr, mem->memory, to_copy);
	mem->size -= to_copy;
	mem->memory += to_copy;
	return to_copy;
}

/* This callback will be used to cancel an ongoing request, 
 * see http://curl.haxx.se/docs/faq.html#How_do_I_stop_an_ongoing_transfe */
static int ctw_progress_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow){
	(void) dltotal;
	(void) dlnow;
	(void) ultotal;
	(void) ulnow;

	struct _ctw *common = clientp; 

	return (common->locked == 0);

}

static void cleanup_thread(void *args) {
	struct _ctw *common = args; 
	common->locked = 0;
	post("Cancellation requested.");
}

static void *ctw_exec_req(void *thread_args) {
	struct _ctw *common = thread_args; 
	CURL *curl_handle;
	CURLcode result;
	struct curl_slist *slist = NULL;
	struct _memory_struct in_memory;
	struct _memory_struct out_memory;
	long http_status;
	t_atom http_status_data[3];

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	curl_handle = curl_easy_init();
	if (!curl_handle) {
		MYERROR("Cannot init curl.");
	} else {
		if (common->http_headers != NULL) {
			struct _strlist *header = common->http_headers;
			while(header != NULL) {
				slist = curl_slist_append(slist, header->str);
				header = header->next;
			}
			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, slist);
		}

		curl_easy_setopt(curl_handle, CURLOPT_URL, common->complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, common->timeout);
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, common->sslcheck);
#ifdef NEEDS_CERT_PATH
		if (common->sslcheck){
			curl_easy_setopt(curl_handle, CURLOPT_CAINFO, common->cert_path);
			curl_easy_setopt(curl_handle, CURLOPT_CAPATH, common->cert_path);
		}
#endif
		if (common->auth_token_len) {
			curl_easy_setopt(curl_handle, CURLOPT_COOKIE, common->auth_token);
		}
		if (strcmp(common->req_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, ctw_read_mem_cb);
			/* Prepare data for reading */
			if (common->parameters_len) {
				in_memory.memory = getbytes(strlen(common->parameters) + 1);
				in_memory.size = strlen(common->parameters);
				if (in_memory.memory == NULL) {
					MYERROR("not enough memory");
				}
				memcpy(in_memory.memory, common->parameters, strlen(common->parameters));
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
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, ctw_write_mem_cb);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_memory);
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, ctw_progress_cb);
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, (void *)&common);

		pthread_cleanup_push(cleanup_thread, (void *)common);
		pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, 0);
		result = curl_easy_perform(curl_handle);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
		pthread_cleanup_pop(0);

		/* output status */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_status);
		SETSYMBOL(&http_status_data[0], gensym(common->req_type));
		if (http_status >= 200 && http_status < 300) {
			SETSYMBOL(&http_status_data[1], gensym("bang"));
			outlet_list(common->stat_out, &s_list, 2, &http_status_data[0]);
			if (result == CURLE_OK) {
				outlet_symbol(common->x_ob.ob_outlet, gensym(out_memory.memory));
				/* Free memory */
				string_free(out_memory.memory, &out_memory.size);
				free((void *)result);
			} else {
				post("curl result: %d", result);
				SETFLOAT(&http_status_data[1], (float)http_status);
				SETSYMBOL(&http_status_data[2], gensym(curl_easy_strerror(result)));
				pd_error(thread_args, "Error while performing request: %s", curl_easy_strerror(result));
				outlet_list(common->stat_out, &s_list, 3, &http_status_data[0]);
			}
		} else {
			SETFLOAT(&http_status_data[1], (float)http_status);
			SETFLOAT(&http_status_data[2], (float)result);
			pd_error(thread_args, "HTTP error while performing request: %li", http_status);
			outlet_list(common->stat_out, &s_list, 3, &http_status_data[0]);
		}
		curl_easy_cleanup(curl_handle);
		string_free(common->complete_url, &common->complete_url_len);
		string_free(common->parameters, &common->parameters_len);
		if (slist != NULL) {
			curl_slist_free_all(slist);
		}
	}
	common->locked = 0;
	return NULL;
}

static void ctw_thread_exec(void *x, void *(*func) (void *)) {
	int rc;
	struct _ctw *common = x;
	pthread_attr_t thread_attributes;

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&(common->thread), &thread_attributes, func, (void *)x);
	pthread_attr_destroy(&thread_attributes);
	if (rc) {
		MYERROR("Could not create thread with code %d", rc);
		string_free(common->complete_url, &common->complete_url_len);
		string_free(common->parameters, &common->parameters_len);
		common->locked = 0;
	}
}

static void ctw_set_sslcheck(struct _ctw *x, int val) {
	if (val != 0) {
		x->sslcheck = 1;
	} else {
		x->sslcheck = 0;
	}
}

static void ctw_cancel(struct _ctw *x) {
	pthread_cancel(x->thread);
}

static void ctw_add_header(void *x, int argc, t_atom *argv){
	struct _ctw *common = x;
	char *val;
	char temp[MAXPDSTRING];
	int i;
	size_t header_len = 0;
	size_t val_len;
	if (argc < 1) {
		pd_error(x, "You need to add some data to set headers");
		return;
	}
	for (i = 0; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		header_len +=strlen(temp) + 1;
	}
	val = string_create(&(val_len), header_len);
	for (i = 0; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		strcat(val, temp);
		if (i < argc -1) {
			strcat(val, " ");
		}
	}
	common->http_headers = strlist_add(common->http_headers, val, val_len);
}

static void ctw_clear_headers(struct _ctw *x) {
	strlist_free(x->http_headers);
	x->http_headers = NULL;
}

static void ctw_set_timeout(struct _ctw *x, int val) {
	x->timeout = (long) val;
}

static void ctw_init(struct _ctw *x) {
	curl_global_init(CURL_GLOBAL_ALL);
	x->base_url_len = 0;
	x->parameters_len = 0;
	x->complete_url_len = 0;
	x->auth_token_len = 0;
	x->http_headers = NULL;

	ctw_set_timeout(x, 0);
	ctw_set_sslcheck(x, 1);
}

static void ctw_free(struct _ctw *x) {
	string_free(x->base_url, &x->base_url_len);
	string_free(x->parameters, &x->parameters_len);
	string_free(x->complete_url, &x->complete_url_len);
	string_free(x->auth_token, &x->auth_token_len);
	ctw_clear_headers(x);
	curl_global_cleanup();
#ifdef NEEDS_CERT_PATH
	string_free(x->cert_path, &x->cert_path_len);
#endif 
}

#ifdef NEEDS_CERT_PATH
static void ctw_set_cert_path(struct _ctw *x, char *directory) {
	x->cert_path = string_create(&x->cert_path_len, strlen(directory) + 11);
	strcpy(x->cert_path, directory);
	size_t i;
	for(i = 0; i < strlen(x->cert_path); i++) {
		if (x->cert_path[i] == '/') {
			x->cert_path[i] = '\\';
		}
	}
	strcat(x->cert_path, "\\cacert.pem");
}
#endif 
