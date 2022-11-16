/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2022 Thomas Mayer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#define OFF 0
#define ON 1
#define MODE_BLOCKING 0
#define MODE_STREAMING 1

struct _memory_struct {
	char *memory;
	size_t size;
};

struct _ctw {
	t_object x_ob;
	t_outlet *data_out;
	t_outlet *error_out;
	t_atom *out;
	t_canvas *x_canvas; /* needed for getting file path */
	pthread_t thread;
	struct _strlist *http_headers;
	char req_type[REQUEST_TYPE_LEN]; /*One of GET, PUT, POST, DELETE, PATCH, HEAD, OPTIONS, CONNECT, TRACE*/
	size_t base_url_len;
	char *base_url;
	size_t parameters_len;
	char *parameters; /* POST, PUT, PATCH parameters */
	size_t complete_url_len;
	char *complete_url;
	size_t auth_token_len;
	char *auth_token; /* for cookie authentication */
	long timeout;
	size_t out_file_len;
	char *out_file; /* filename for output, if any */
	size_t proxy_len;
	char *proxy; /* proxy url, if any*/
	size_t proxy_user_len;
	char *proxy_user; /* username for proxy, if any*/
	size_t proxy_pass_len;
	char *proxy_pass; /* password for proxy, if any */
	CURLM *multi_handle;
	CURL *easy_handle;
	unsigned char locked; /* is object locked? */
	unsigned char sslcheck; /* check SSL certificate with CA list? */
	unsigned char mode; /* output when done or stream? */
	unsigned char clear_cb; /* clear callback. Used, when streaming output and thread is running */
#ifdef _WIN32
	/* pthread_cancel will not exit the thread on Windows.
	 * Therefore a different approach is needed: 
	 * Set a variable to the object that is read in the POSIX thread.
	 * After reading the variable, the thread itself will call pthread_exit().
	 * This is considerably slower, sorry about that. */
	unsigned char exit_thread; /* signal exiting the thread on Windows */
#endif
#ifdef PDINSTANCE
	t_pdinstance *x_pd_this;  /**< pointer to the owner pd instance */
#endif
};

/* used for writing data on HTTP data received:
   if output on completely received, write to mem
   if output in streaming mode, output at outlet of ctw */
struct _cb_val {
	struct _memory_struct *mem;
	struct _ctw *ctw;
};

/* collecting output data */
static size_t ctw_write_mem(const void *ptr, const size_t realsize, struct _memory_struct *mem);
/* output as stream */
static size_t ctw_write_stream(const void *ptr, const size_t realsize, struct _ctw *ctw);
/* callback for writing date from libcurl */
static size_t ctw_write_mem_cb(const void *ptr, size_t size, size_t nmemb, void *data);
/* callback for reading data */
static size_t ctw_read_mem_cb(void *ptr, size_t size, size_t nmemb, void *data);
/* helper for setting string data */
static char *ctw_set_param(struct _ctw *common, t_atom *arg, size_t *string_len, char *error_msg);
/* aborts request thread */
static void ctw_abort_request_thread(void *args);
/* prepares for HTTP request, all verbs */
static void ctw_prepare_basic(struct _ctw *common, struct curl_slist *slist);
/* prepares for PUT request */
static void ctw_prepare_put(struct _ctw *common, struct _memory_struct *in_memory);
/* prepares for POST request */
static void ctw_prepare_post(struct _ctw *common);
/* prepares for DELETE request */
static void ctw_prepare_delete(struct _ctw *common);
/* prepares for HEAD request */
static void ctw_prepare_head(struct _ctw *common);
/* prepares for PATCH request */
static void ctw_prepare_patch(struct _ctw *common, struct _memory_struct *in_memory);
/* prepares for OPTIONS request */
static void ctw_prepare_options(struct _ctw *common);
/* prepares for CONNECT request */
static void ctw_prepare_connect(struct _ctw *common);
/* prepares for TRACE request */
static void ctw_prepare_trace(struct _ctw *common, struct curl_slist *slist);
/* prepares for HTTP request, setting in- and output */
static FILE *ctw_prepare(struct _ctw *common, struct curl_slist *slist,
		struct _memory_struct *out_memory, struct _memory_struct *in_memory);
/* checking curl status when setting options */
static void ctw_libcurl_option_status_check(struct _ctw *common, CURLcode code);
/* curl request loop */
static int ctw_libcurl_loop(struct _ctw *common);
/* performes the HTTP request */
static void ctw_perform(struct _ctw *common);
/* prepares performing HTTP request in separate thread */
static void ctw_thread_perform(struct _ctw *common);
/* outputs curl error */
static void ctw_output_curl_error(struct _ctw *common, CURLMsg *msg);
/* outputs collected data and bang */
static void ctw_output(struct _ctw *common, struct _memory_struct *out_memory, FILE *fp);
/* cleans up after HTTP request */
static void ctw_cleanup_request(struct _ctw *common, FILE *fp, struct curl_slist *slist);
/* executes HTTP request */
static void *ctw_exec(void *thread_args);
/* executes HTTP request in thread */
static void ctw_thread_exec(struct _ctw *x, void *(*func) (void *));
/* checks for valid HTTP verb */
static int ctw_check_request_type(const char *req_type);
/* sets checking for SSL certificate */
static void ctw_set_sslcheck(struct _ctw *common, int val);
/* cancels HTTP request */
static void ctw_cancel(struct _ctw *common);
/* adds HTTP header */
static void ctw_add_header(struct _ctw *common, int argc, t_atom *argv);
/* clears HTTP header */
static void ctw_clear_headers(struct _ctw *common);
/* sets output file */
static void ctw_set_file(struct _ctw *common, int argc, t_atom *argv);
/* sets timeout for request */
static void ctw_set_timeout(struct _ctw *common, int val);
/* sets mode to blocking or streaming */
static void ctw_set_mode(struct _ctw *common, int argc, t_atom *argv);
/* sets mode to blocking or streaming as numerical value */
static void ctw_set_mode_number(struct _ctw *common, int val);
/* sets proxy */
static void ctw_set_proxy(struct _ctw *common, int argc, t_atom *argv);
/* inits object */
static void ctw_init(struct _ctw *common);
/* frees data */
static void ctw_free(struct _ctw *common);

/* begin implementations */
static size_t ctw_write_mem(const void *const ptr, const size_t realsize, struct _memory_struct *const mem) {
	mem->memory = resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
	if (mem->memory == NULL) {
		sys_lock();
		pd_error(0, "not enough memory.");
		sys_unlock();
		return 0;
	}
	memcpy(&mem->memory[mem->size], ptr, realsize);
	if (mem->size + realsize - mem->size != realsize) {
		sys_lock();
		pd_error(0, "Integer overflow or similar. Bad Things can happen.");
		sys_unlock();
	}
	mem->size += realsize;
	mem->memory[mem->size] = '\0';
	return realsize;
}

static size_t ctw_write_stream(const void *const ptr, const size_t realsize, struct _ctw *const ctw) {
	char *stream_output = getbytes(realsize + sizeof(char));
	if (stream_output == NULL) {
#ifdef PDINSTANCE
		pd_setinstance(ctw->x_pd_this);
#endif
		sys_lock();
		pd_error(ctw, "not enough memory");
		sys_unlock();
		return 0;
	}
	memcpy(stream_output, ptr, realsize);
	stream_output[realsize] = '\0';
#ifdef PDINSTANCE
	pd_setinstance(ctw->x_pd_this);
#endif
	sys_lock();
	outlet_symbol(ctw->data_out, gensym(stream_output));
	sys_unlock();
	/* Free memory */
	freebytes(stream_output, realsize + sizeof(char));
	return realsize;
}

static size_t ctw_write_mem_cb(const void *const ptr, const size_t size, const size_t nmemb, void *const data) {
	const size_t realsize = size * nmemb;
	struct _cb_val *const cb_val = data;
	struct _ctw *const ctw = cb_val->ctw;
	if (ctw->clear_cb == ON) {
		return -1;
	}

	if (ctw->mode == MODE_BLOCKING) {
		return ctw_write_mem(ptr, realsize, cb_val->mem);
	} else if (ctw->mode == MODE_STREAMING) {
		return ctw_write_stream(ptr, realsize, ctw);
	}
	return 0;
}

static size_t ctw_read_mem_cb(void *const ptr, const size_t size, const size_t nmemb, void *const data) {
	const size_t realsize = size * nmemb;
	struct _memory_struct *const mem = data;
	const size_t to_copy = (mem->size < realsize) ? mem->size : realsize;

	memcpy(ptr, mem->memory, to_copy);
	mem->size -= to_copy;
	mem->memory += to_copy;
	return to_copy;
}

static char *ctw_set_param(struct _ctw *const common, t_atom *const arg, size_t *const string_len, char *const error_msg) {
	char temp[MAXPDSTRING];
	char *string;

	if (arg[0].a_type != A_SYMBOL) {
#ifdef PDINSTANCE
		pd_setinstance(common->x_pd_this);
#endif
		sys_lock();
		pd_error(common, "%s", error_msg);
		sys_unlock();
		return NULL;
	} 
	atom_string(arg, temp, MAXPDSTRING);
	string = string_create(string_len, strlen(temp));
	if (string == NULL) {
		return NULL;
	}
	strcpy(string, temp);
	return string;
}

static void ctw_abort_request_thread(void *const args) {
	struct _ctw *const common = args;
	if (common->locked == OFF) {
		return;
	}
	curl_multi_remove_handle(common->multi_handle, common->easy_handle);
	common->locked = OFF;
#ifdef _WIN32
	common->exit_thread = OFF;
#endif
	post("request cancelled.");
}

static void ctw_prepare_basic(struct _ctw *const common, struct curl_slist *slist) {
	/* enable redirection */
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_FOLLOWLOCATION, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_AUTOREFERER, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_MAXREDIRS, 30));

	if (common->http_headers != NULL) {
		struct _strlist *header = common->http_headers;
		while(header != NULL) {
			slist = curl_slist_append(slist, header->str);
			header = header->next;
		}
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_HTTPHEADER, slist));
	}

	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_URL, common->complete_url));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_NOSIGNAL, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_TIMEOUT_MS, common->timeout));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_SSL_VERIFYPEER, common->sslcheck));
	if (common->auth_token_len) {
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_COOKIE, common->auth_token));
	}
	if(common->proxy_len) {
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_PROXY, common->proxy));
	}
	if(common->proxy_user_len) {
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_PROXYUSERNAME, common->proxy_user));
	}
	if(common->proxy_pass_len) {
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_PROXYPASSWORD, common->proxy_pass));
	}
}

static void ctw_prepare_put(struct _ctw *const common, struct _memory_struct *const in_memory) {
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_UPLOAD, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_READFUNCTION, ctw_read_mem_cb));
	/* Prepare data for reading */
	if (common->parameters_len) {
		(*in_memory).memory = getbytes(strlen(common->parameters) + 1);
		(*in_memory).size = strlen(common->parameters);
		if ((*in_memory).memory == NULL) {
#ifdef PDINSTANCE
			pd_setinstance(common->x_pd_this);
#endif
			sys_lock();
			pd_error(common, "not enough memory.");
			sys_unlock();
			return;
		}
		memcpy((*in_memory).memory, common->parameters, strlen(common->parameters));
	} else {
		(*in_memory).memory = NULL;
		(*in_memory).size = 0;
	}
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_READDATA, (void *)in_memory));
}

static void ctw_prepare_post(struct _ctw *const common) {
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_POST, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_POSTFIELDS, common->parameters));
}

static void ctw_prepare_delete(struct _ctw *const common) {
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "DELETE"));
}

static void ctw_prepare_head(struct _ctw *const common) {
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "HEAD"));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_HEADER, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_NOBODY, 1));
}

static void ctw_prepare_patch(struct _ctw *const common, struct _memory_struct *const in_memory) {
	ctw_prepare_put(common, in_memory);
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "PATCH"));
}

static void ctw_prepare_options(struct _ctw *const common) {
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "OPTIONS"));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_HEADER, 1));
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_NOBODY, 1));
}

static void ctw_prepare_connect(struct _ctw *const common) {
	/* TODO: Connect */
	(void) common;
}

static void ctw_prepare_trace(struct _ctw *const common, struct curl_slist *slist) {
	slist = curl_slist_append(slist, "Content-type: message/http");
	ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "TRACE"));
}

static FILE *ctw_prepare(struct _ctw *const common, struct curl_slist *const slist,
		struct _memory_struct *const out_memory, struct _memory_struct *const in_memory) {
	FILE *fp = NULL; 

	ctw_prepare_basic(common, slist);

	if (strcmp(common->req_type, "PUT") == 0) {
		ctw_prepare_put(common, in_memory);
	} else if (strcmp(common->req_type, "POST") == 0) {
		ctw_prepare_post(common);
	} else if (strcmp(common->req_type, "DELETE") == 0) {
		ctw_prepare_delete(common);
	} else if (strcmp(common->req_type, "HEAD") == 0) {
		ctw_prepare_head(common);
	} else if (strcmp(common->req_type, "PATCH") == 0) {
		ctw_prepare_patch(common, in_memory);
	} else if (strcmp(common->req_type, "OPTIONS") == 0) {
		ctw_prepare_options(common);
	} else if (strcmp(common->req_type, "CONNECT") == 0) {
		ctw_prepare_connect(common);
	} else if (strcmp(common->req_type, "TRACE") == 0) {
		ctw_prepare_trace(common, slist);
	}
	(*out_memory).memory = getbytes(1);
	(*out_memory).size = 0;
	if (common->out_file_len) {
		if ((fp = fopen(common->out_file, "wb"))) {
			ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_WRITEDATA, (void *)fp));
		} else {
#ifdef PDINSTANCE
			pd_setinstance(common->x_pd_this);
#endif
			sys_lock();
			pd_error(common, "%s: writing not possible. Will output on left outlet instead.", 
					common->out_file);
			sys_unlock();
		}
	}
	if (fp == NULL) {
		struct _cb_val *cb_val = getbytes(sizeof(struct _cb_val));
		cb_val->mem = out_memory;
		cb_val->ctw = common;
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_WRITEFUNCTION, ctw_write_mem_cb));
		ctw_libcurl_option_status_check(common, curl_easy_setopt(common->easy_handle, CURLOPT_WRITEDATA, (void *)cb_val));
	}
	curl_multi_add_handle(common->multi_handle, common->easy_handle);
	return fp;
}

static void ctw_libcurl_option_status_check(struct _ctw *common, CURLcode code){
	if (code != CURLE_OK){
		pd_error(common, "%s", curl_easy_strerror(code));
	}
}

static int ctw_libcurl_loop(struct _ctw *const common) {
	CURLMcode code;
	int numfds;
	int running = 1;

	code = curl_multi_perform(common->multi_handle, &running);
#ifdef _WIN32
	if (common->exit_thread == ON){
		ctw_abort_request_thread(common); 
		pthread_exit(NULL);
	}
#endif
	if (code != CURLM_OK) {
#ifdef PDINSTANCE
		pd_setinstance(common->x_pd_this);
#endif
		sys_lock();
		pd_error(common, "Error while performing request: %s", curl_multi_strerror(code));
		sys_unlock();
	} else {
		/* wait for activity or timeout */
		code = curl_multi_poll(common->multi_handle, NULL, 0, 1000, &numfds);
		if (code != CURLM_OK) {
#ifdef PDINSTANCE
			pd_setinstance(common->x_pd_this);
#endif
			sys_lock();
			pd_error(common, "Error while performing request: %s", curl_multi_strerror(code));
			sys_unlock();
		}
	}
	return running;
}

static void ctw_perform(struct _ctw *const common) {
	int running;
	do {
		running = ctw_libcurl_loop(common);
	} while (running);
#ifdef _WIN32
	common->exit_thread = OFF;
#endif
}

static void ctw_thread_perform(struct _ctw *const common) {
	pthread_cleanup_push(ctw_abort_request_thread, (void *)common);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	ctw_perform(common);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	pthread_cleanup_pop(0);
}

static void ctw_output_curl_error(struct _ctw *const common, CURLMsg *const msg) {
	t_atom status_data[2];

#ifdef PDINSTANCE
	pd_setinstance(common->x_pd_this);
#endif
	sys_lock();
	SETFLOAT(&status_data[0], msg->data.result);
	SETSYMBOL(&status_data[1], gensym(curl_easy_strerror(msg->data.result)));
	pd_error(common, "Error while performing request: %s", curl_easy_strerror(msg->data.result));
	outlet_list(common->error_out, &s_list, 2, &status_data[0]);
	sys_unlock();
}

static void ctw_output(struct _ctw *const common, struct _memory_struct *const out_memory, FILE *const fp) {
	CURLMsg *msg;
	int msgs_left;

	while ((msg = curl_multi_info_read(common->multi_handle, &msgs_left))) {
		if (msg->msg == CURLMSG_DONE) {
			long http_status;
			/* output status */
			curl_easy_getinfo(common->easy_handle, CURLINFO_RESPONSE_CODE, &http_status);
			if (http_status >= 200 && http_status < 300) {
				if (msg->data.result == CURLE_OK) {
#ifdef PDINSTANCE
					pd_setinstance(common->x_pd_this);
#endif
					sys_lock();
					if (fp == NULL) {
						outlet_symbol(common->data_out, gensym((*out_memory).memory));
					}
					outlet_bang(common->x_ob.ob_outlet);
					sys_unlock();
					/* Free memory */
					string_free((*out_memory).memory, &(*out_memory).size);
				} else {
					ctw_output_curl_error(common, msg);
				}
			} else {
				if (msg->data.result == CURLE_OK){
					t_atom http_status_data;
#ifdef PDINSTANCE
					pd_setinstance(common->x_pd_this);
#endif
					sys_lock();
					SETFLOAT(&http_status_data, (float)http_status);
					pd_error(common, "HTTP error while performing request: %li.", http_status);
					outlet_float(common->error_out, atom_getfloat(&http_status_data));
					sys_unlock();
				} else {
					ctw_output_curl_error(common, msg);
				}
			}
			curl_easy_cleanup(common->easy_handle);
			curl_multi_cleanup(common->multi_handle);
			break;
		}
	}
}

static void ctw_cleanup_request(struct _ctw *const common, FILE *const fp, struct curl_slist *const slist) {
	string_free(common->complete_url, &common->complete_url_len);
	string_free(common->parameters, &common->parameters_len);
	if (slist != NULL) {
		curl_slist_free_all(slist);
	}
	if (fp) {
		fclose(fp);
	}
	common->locked = OFF;
}

static void *ctw_exec(void *const thread_args) {
	struct _ctw *const common = thread_args; 

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	common->easy_handle = curl_easy_init();
	common->multi_handle = curl_multi_init();
	if (common->easy_handle == NULL) {
#ifdef PDINSTANCE
		pd_setinstance(common->x_pd_this);
#endif
		sys_lock();
		pd_error(common, "Cannot init curl.");
		sys_unlock();
		ctw_cleanup_request(common, NULL, NULL);
	} else {
		struct curl_slist *slist = NULL;
		struct _memory_struct out_memory;
		struct _memory_struct in_memory;
		FILE *fp = ctw_prepare(common, slist, &out_memory, &in_memory);
		ctw_thread_perform(common);
		ctw_output(common, &out_memory, fp);
		ctw_cleanup_request(common, fp, slist);
	}
	return NULL;
}

static void ctw_thread_exec(struct _ctw *const common, void *(*func) (void *)) {
	int rc;
	pthread_attr_t thread_attributes;

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&(common->thread), &thread_attributes, func, (void *)common);
	pthread_attr_destroy(&thread_attributes);
	if (rc) {
		pd_error(common, "Could not create thread with code %d.", rc);
		string_free(common->complete_url, &common->complete_url_len);
		string_free(common->parameters, &common->parameters_len);
		common->locked = OFF;
	}
}

static int ctw_check_request_type(const char *const req_type) {
	return (strcmp(req_type, "GET")
			&& strcmp(req_type, "POST")
			&& strcmp(req_type, "HEAD")
			&& strcmp(req_type, "PUT")
			&& strcmp(req_type, "DELETE")
			&& strcmp(req_type, "PATCH")
			&& strcmp(req_type, "OPTIONS")
			&& strcmp(req_type, "CONNECT")
			&& strcmp(req_type, "TRACE"));
}

static void ctw_set_sslcheck(struct _ctw *const common, const int val) {
	if (val != OFF) {
		common->sslcheck = ON;
	} else {
		common->sslcheck = OFF;
	}
}

static void ctw_cancel(struct _ctw *const common) {
#ifndef _WIN32
	int cancel_error;
#endif
	if (common->locked == OFF) {
		return;
	}
#ifdef _WIN32
	common->exit_thread = ON;
#else
	cancel_error = pthread_cancel(common->thread);

	if (cancel_error) {
		pd_error(common, "Error cancelling: %s", strerror(cancel_error));
	}
#endif
}

static void ctw_add_header(struct _ctw *const common, const int argc, t_atom *const argv) {
	char *val;
	char temp[MAXPDSTRING];
	size_t header_len = 0;
	size_t val_len;
	if (argc < 1) {
		pd_error(common, "You need to add some data to set headers.");
		return;
	}
	for (int i = 0; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		header_len += strlen(temp) + 1;
	}
	val = string_create(&(val_len), header_len);
	for (int i = 0; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		strcat(val, temp);
		if (i < argc - 1) {
			strcat(val, " ");
		}
	}
	common->http_headers = strlist_add(common->http_headers, val, val_len);
}

static void ctw_clear_headers(struct _ctw *const common) {
	strlist_free(common->http_headers);
	common->http_headers = NULL;
}

static void ctw_set_file(struct _ctw *const common, const int argc, t_atom *const argv) {
	t_symbol *filename;
	char buf[MAXPDSTRING];

	string_free(common->out_file, &common->out_file_len);
	if (argc == 0) {
		return;
	}
	filename = atom_getsymbol(argv);
	if (filename == 0) {
		pd_error(common, "not a filename.");
		return;
	}
	canvas_makefilename(common->x_canvas, filename->s_name, buf, MAXPDSTRING);
	common->out_file = string_create(&(common->out_file_len), strlen(buf));
	strcpy(common->out_file, buf);
}

static void ctw_set_timeout(struct _ctw *const common, const int val) {
	common->timeout = (long) val;
}

static void ctw_set_mode_number(struct _ctw *common, int val) {
	common->mode = val;
}

static void ctw_set_mode(struct _ctw *common, int argc, t_atom *argv) {
	t_symbol *mode;

	if (argc != 1) {
		pd_error(common, "mode needs a name");
		return;
	}
	mode = atom_getsymbol(argv);
	if (strcmp(mode->s_name, "block") == 0) {
		ctw_set_mode_number(common, OFF);
	} else if (strcmp(mode->s_name, "stream") == 0) {
		ctw_set_mode_number(common, ON);
	} else {
		pd_error(common, "not a valid mode");
	}
}

static void ctw_set_proxy(struct _ctw *const common, const int argc, t_atom *const argv) {
	char tmp[MAXPDSTRING];

	string_free(common->proxy, &common->proxy_len);
	string_free(common->proxy_user, &common->proxy_user_len);
	string_free(common->proxy_pass, &common->proxy_pass_len);
	switch(argc) {
		case 3:
			atom_string(argv + 1, tmp, MAXPDSTRING);
			common->proxy_user = string_create(&(common->proxy_user_len), strlen(tmp));
			strcpy(common->proxy_user, tmp);
			atom_string(argv + 2, tmp, MAXPDSTRING);
			common->proxy_pass = string_create(&(common->proxy_pass_len), strlen(tmp));
			strcpy(common->proxy_pass, tmp);
			/* FALLTHRU */
		case 1:
			atom_string(argv, tmp, MAXPDSTRING);
			common->proxy = string_create(&(common->proxy_len), strlen(tmp));
			strcpy(common->proxy, tmp);
		case 0:
			break;
		default:
			pd_error(common, "proxy must have exactly 0, 1 or 3 parameters");
			break;
	}
}

static void ctw_init(struct _ctw *const common) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	common->base_url_len = 0;
	common->parameters_len = 0;
	common->complete_url_len = 0;
	common->auth_token_len = 0;
	common->http_headers = NULL;
	common->out_file_len = 0;
	common->proxy_len = 0;
	common->proxy_user_len = 0;
	common->proxy_pass_len = 0;
	common->x_canvas = canvas_getcurrent();
	common->clear_cb = OFF;
#ifdef _WIN32
	common->exit_thread = OFF;
#endif
#ifdef PDINSTANCE
	common->x_pd_this = pd_this;
#endif

	ctw_set_timeout(common, 0);
	ctw_set_mode_number(common, MODE_BLOCKING);
	ctw_set_sslcheck(common, ON);
}

static void ctw_free(struct _ctw *const common) {
	common->clear_cb = ON;
	if (common->locked == ON) {
		pthread_cancel(common->thread);
	}
	string_free(common->base_url, &common->base_url_len);
	string_free(common->parameters, &common->parameters_len);
	string_free(common->complete_url, &common->complete_url_len);
	string_free(common->auth_token, &common->auth_token_len);
	string_free(common->out_file, &common->out_file_len);
	string_free(common->proxy, &common->proxy_len);
	string_free(common->proxy_user, &common->proxy_user_len);
	string_free(common->proxy_pass, &common->proxy_pass_len);
	ctw_clear_headers(common);
	curl_global_cleanup();
}
