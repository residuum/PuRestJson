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
	char cert_path[2048];
#endif
};

static size_t ctw_write_mem_cb(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct _memory_struct *mem = data;

	mem->memory = (char *) resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
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

static void *ctw_exec_req(void *thread_args) {
	struct _ctw *common = thread_args; 
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
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, common->sslcheck);
#ifdef NEEDS_CERT_PATH
		if (common->sslcheck){
			curl_easy_setopt(curl_handle, CURLOPT_SSLCERT, common->cert_path);
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
				string_free(out_memory.memory, &out_memory.size);
				free((void *)result);
			} else {
				SETFLOAT(&http_status_data[1], (float)http_status);
				SETFLOAT(&http_status_data[2], (float)result);
				outlet_list(common->stat_out, &s_list, 3, &http_status_data[0]);
				MYERROR("Error while performing request: %s", curl_easy_strerror(result));
			}
		} else {
			SETFLOAT(&http_status_data[1], (float)http_status);
			SETFLOAT(&http_status_data[2], (float)result);
			MYERROR("HTTP error while performing request: %li", http_status);
			outlet_list(common->stat_out, &s_list, 3, &http_status_data[0]);
		}
		curl_easy_cleanup(curl_handle);
	} else {
		MYERROR("Cannot init curl.");
	}
	string_free(common->complete_url, &common->complete_url_len);
	string_free(common->parameters, &common->parameters_len);
	common->locked = 0;
	return NULL;
}

static void ctw_thread_exec(struct _ctw *x, void *(*func) (void *)) {
	int rc;
	pthread_t thread;
	pthread_attr_t thread_attributes;

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&thread, &thread_attributes, func, (void *)x);
	pthread_attr_destroy(&thread_attributes);
	if (rc) {
		MYERROR("Could not create thread with code %d", rc);
		string_free(x->complete_url, &x->complete_url_len);
		string_free(x->parameters, &x->parameters_len);
		x->locked = 0;
	}
}

static void ctw_set_sslcheck(struct _ctw *x, int val) {
	if (val != 0) {
		x->sslcheck = 1;
	} else {
		x->sslcheck = 0;
	}
}

static void ctw_set_timeout(struct _ctw *x, int val) {
	x->timeout = (long) val;
}

static void ctw_init(struct _ctw *x) {
	x->base_url_len = 0;
	x->parameters_len = 0;
	x->complete_url_len = 0;
	x->auth_token_len = 0;

	ctw_set_timeout(x, 0);
	ctw_set_sslcheck(x, 1);
}

static void ctw_free(struct _ctw *x) {
	string_free(x->base_url, &x->base_url_len);
	string_free(x->parameters, &x->parameters_len);
	string_free(x->complete_url, &x->complete_url_len);
	string_free(x->auth_token, &x->auth_token_len);
}
