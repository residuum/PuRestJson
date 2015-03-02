/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2015 Thomas Mayer

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

#ifdef NEEDS_CERT_PATH
#include "m_imp.h"
#endif
struct _memory_struct {
	char *memory;
	size_t size;
};

struct _ctw {
	t_object x_ob;
	t_outlet *status_out;
	t_atom *out;
	t_canvas *x_canvas;
	pthread_t thread;
	struct _strlist *http_headers;
	char req_type[REQUEST_TYPE_LEN]; /*One of GET, PUT, POST, DELETE, PATCH, HEAD, OPTIONS, CONNECT, TRACE*/
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
	char *out_file;
	size_t out_file_len;
	CURLM *multi_handle;
	CURL *easy_handle;
	unsigned char mode;
#ifdef NEEDS_CERT_PATH
	size_t cert_path_len;
	char *cert_path;
#endif
};

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
/* cancels request in thread */
static void ctw_cancel_request(void *args);
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
/* inits object */
static void ctw_init(struct _ctw *common);
/* frees data */
static void ctw_free(struct _ctw *common);
#ifdef NEEDS_CERT_PATH
/* sets path to certificate file */
static void ctw_set_cert_path(struct _ctw *common, const char *directory);
#endif

/* begin implementations */
static size_t ctw_write_mem(const void *const ptr, const size_t realsize, struct _memory_struct *const mem) {
	mem->memory = resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
	if (mem->memory == NULL) {
		MYERROR("not enough memory.");
	}
	memcpy(&mem->memory[mem->size], ptr, realsize);
	if (mem->size + realsize - mem->size != realsize) {
		MYERROR("Integer overflow or similar. Bad Things can happen.");
	}
	mem->size += realsize;
	mem->memory[mem->size] = '\0';
	return realsize;
}

static size_t ctw_write_stream(const void *const ptr, const size_t realsize, struct _ctw *const ctw) {
	char *stream_output = getbytes(realsize + sizeof(char));
	if (stream_output == NULL) {
		MYERROR("not enough memory");
	}
	memcpy(stream_output, ptr, realsize);
	stream_output[realsize] = '\0';
	outlet_symbol(ctw->x_ob.ob_outlet, gensym(stream_output));
	/* Free memory */
	freebytes(stream_output, realsize + sizeof(char));
	return realsize;
}

static size_t ctw_write_mem_cb(const void *const ptr, const size_t size, const size_t nmemb, void *const data) {
	const size_t realsize = size * nmemb;
	struct _cb_val *const cb_val = data;
	struct _ctw *const ctw = cb_val->ctw;

	if (ctw->mode == 0) {
		return ctw_write_mem(ptr, realsize, cb_val->mem);
	} else if (ctw->mode == 1) {
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
		pd_error(common, "%s", error_msg);
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

static void ctw_cancel_request(void *const args) {
	struct _ctw *const common = args; 
	curl_multi_remove_handle(common->multi_handle, common->easy_handle);
	common->locked = 0;
	post("request cancelled.");
}

static void ctw_prepare_basic(struct _ctw *const common, struct curl_slist *slist) {
	/* enable redirection */
	curl_easy_setopt (common->easy_handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt (common->easy_handle, CURLOPT_AUTOREFERER, 1);
	curl_easy_setopt (common->easy_handle, CURLOPT_MAXREDIRS, 30);

	if (common->http_headers != NULL) {
		struct _strlist *header = common->http_headers;
		while(header != NULL) {
			slist = curl_slist_append(slist, header->str);
			header = header->next;
		}
		curl_easy_setopt(common->easy_handle, CURLOPT_HTTPHEADER, slist);
	}

	curl_easy_setopt(common->easy_handle, CURLOPT_URL, common->complete_url);
	curl_easy_setopt(common->easy_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(common->easy_handle, CURLOPT_TIMEOUT_MS, common->timeout);
	curl_easy_setopt(common->easy_handle, CURLOPT_SSL_VERIFYPEER, common->sslcheck);
#ifdef NEEDS_CERT_PATH
	if (common->sslcheck){
		curl_easy_setopt(common->easy_handle, CURLOPT_CAINFO, common->cert_path);
		curl_easy_setopt(common->easy_handle, CURLOPT_CAPATH, common->cert_path);
	}
#endif
	if (common->auth_token_len) {
		curl_easy_setopt(common->easy_handle, CURLOPT_COOKIE, common->auth_token);
	}
}

static void ctw_prepare_put(struct _ctw *const common, struct _memory_struct *const in_memory) {
	curl_easy_setopt(common->easy_handle, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(common->easy_handle, CURLOPT_READFUNCTION, ctw_read_mem_cb);
	/* Prepare data for reading */
	if (common->parameters_len) {
		(*in_memory).memory = getbytes(strlen(common->parameters) + 1);
		(*in_memory).size = strlen(common->parameters);
		if ((*in_memory).memory == NULL) {
			MYERROR("not enough memory.");
		}
		memcpy((*in_memory).memory, common->parameters, strlen(common->parameters));
	} else {
		(*in_memory).memory = NULL;
		(*in_memory).size = 0;
	}
	curl_easy_setopt(common->easy_handle, CURLOPT_READDATA, (void *)in_memory);
}

static void ctw_prepare_post(struct _ctw *const common) {
	curl_easy_setopt(common->easy_handle, CURLOPT_POST, 1);
	curl_easy_setopt(common->easy_handle, CURLOPT_POSTFIELDS, common->parameters);
}

static void ctw_prepare_delete(struct _ctw *const common) {
	curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
}

static void ctw_prepare_head(struct _ctw *const common) {
	curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "HEAD");
	curl_easy_setopt(common->easy_handle, CURLOPT_HEADER, 1);
	curl_easy_setopt(common->easy_handle, CURLOPT_NOBODY, 1); 
}

static void ctw_prepare_patch(struct _ctw *const common, struct _memory_struct *const in_memory) {
	ctw_prepare_put(common, in_memory);
	curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
}

static void ctw_prepare_options(struct _ctw *const common) {
	curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "OPTIONS");
	curl_easy_setopt(common->easy_handle, CURLOPT_HEADER, 1);
	curl_easy_setopt(common->easy_handle, CURLOPT_NOBODY, 1); 
}

static void ctw_prepare_connect(struct _ctw *const common) {
	/* TODO: Connect */
	(void) common;
}

static void ctw_prepare_trace(struct _ctw *const common, struct curl_slist *slist) {
	slist = curl_slist_append(slist, "Content-type: message/http");
	curl_easy_setopt(common->easy_handle, CURLOPT_CUSTOMREQUEST, "TRACE");
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
			curl_easy_setopt(common->easy_handle, CURLOPT_WRITEDATA, (void *)fp);
		} else {
			pd_error(common, "%s: writing not possible. Will output on left outlet instead.", 
					common->out_file);
		}
	}
	if (fp == NULL) {
		struct _cb_val *cb_val = getbytes(sizeof(struct _cb_val));
		cb_val->mem = out_memory;
		cb_val->ctw = common;
		curl_easy_setopt(common->easy_handle, CURLOPT_WRITEFUNCTION, ctw_write_mem_cb);
		curl_easy_setopt(common->easy_handle, CURLOPT_WRITEDATA, (void *)cb_val);
	}
	curl_multi_add_handle(common->multi_handle, common->easy_handle);
	return fp;
}

static int ctw_libcurl_loop(struct _ctw *const common) {
	CURLMcode code;
	struct timeval timeout;
	int rc;
	fd_set fdread;
	fd_set fdwrite;
	fd_set fdexcep;
	int maxfd = -1;
	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);
	timeout.tv_sec = 0;
	timeout.tv_usec = 300;
	int running = 1;

	code = curl_multi_fdset(common->multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
	if (code != CURLM_OK) {
		pd_error(common, "Error while performing request: %s", curl_multi_strerror(code));
	}
	rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
	switch(rc) {
		case -1:
			pd_error(common, "Unspecified error while performing request (network disconnect?).");
			running = 0;
			break;
		case 0: /* timeout */ 
		default: /* action */ 
			code = curl_multi_perform(common->multi_handle, &running);
			if (code != CURLM_OK) {
				pd_error(common, "Error while performing request: %s", curl_multi_strerror(code));
			}
			break;
	}
	return running;
}

static void ctw_perform(struct _ctw *const common) {
	int running;
	const CURLMcode code = curl_multi_perform(common->multi_handle, &running);

	if (code != CURLM_OK) {
		pd_error(common, "Error while performing request: %s", curl_multi_strerror(code));
	}
	do {
		running = ctw_libcurl_loop(common);
	} while (running);
}

static void ctw_thread_perform(struct _ctw *const common) {
	pthread_cleanup_push(ctw_cancel_request, (void *)common);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	ctw_perform(common);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	pthread_cleanup_pop(0);
}

static void ctw_output_curl_error(struct _ctw *const common, CURLMsg *const msg) {
	t_atom status_data[2];

	SETFLOAT(&status_data[0], msg->data.result);
	SETSYMBOL(&status_data[1], gensym(curl_easy_strerror(msg->data.result)));
	pd_error(common, "Error while performing request: %s", curl_easy_strerror(msg->data.result));
	outlet_list(common->status_out, &s_list, 2, &status_data[0]);
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
					if (fp == NULL) {
						outlet_symbol(common->x_ob.ob_outlet, gensym((*out_memory).memory));
					}
					/* Free memory */
					string_free((*out_memory).memory, &(*out_memory).size);
					outlet_bang(common->status_out);
				} else {
					ctw_output_curl_error(common, msg);
				}
			} else {
				if (msg->data.result == CURLE_OK){
					t_atom http_status_data;
					SETFLOAT(&http_status_data, (float)http_status);
					pd_error(common, "HTTP error while performing request: %li.", http_status);
					outlet_float(common->status_out, atom_getfloat(&http_status_data));
				} else {
					ctw_output_curl_error(common, msg);
				}
			}
			curl_easy_cleanup(common->easy_handle);
			curl_multi_cleanup(common->multi_handle);
		}
	}
}

static void *ctw_exec(void *const thread_args) {
	struct _ctw *const common = thread_args; 

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	common->easy_handle = curl_easy_init();
	common->multi_handle = curl_multi_init();
	if (common->easy_handle == NULL) {
		MYERROR("Cannot init curl.");
	} else {
		struct curl_slist *slist = NULL;
		struct _memory_struct out_memory;
		struct _memory_struct in_memory;
		FILE *fp; 
		fp = ctw_prepare(common, slist, &out_memory, &in_memory);
		ctw_thread_perform(common);
		ctw_output(common, &out_memory, fp);
		string_free(common->complete_url, &common->complete_url_len);
		string_free(common->parameters, &common->parameters_len);
		if (slist != NULL) {
			curl_slist_free_all(slist);
		}
		if (fp) {
			fclose(fp);
		}

	}
	common->locked = 0;
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
		MYERROR("Could not create thread with code %d.", rc);
		string_free(common->complete_url, &common->complete_url_len);
		string_free(common->parameters, &common->parameters_len);
		common->locked = 0;
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
	if (val != 0) {
		common->sslcheck = 1;
	} else {
		common->sslcheck = 0;
	}
}

static void ctw_cancel(struct _ctw *const common) {
	pthread_cancel(common->thread);
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
		ctw_set_mode_number(common, 0);
	} else if (strcmp(mode->s_name, "stream") == 0) {
		ctw_set_mode_number(common, 1);
	} else {
		pd_error(common, "not a valid mode");
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
	common->x_canvas = canvas_getcurrent();

	ctw_set_timeout(common, 0);
	ctw_set_mode_number(common, 0);
	ctw_set_sslcheck(common, 1);
}

static void ctw_free(struct _ctw *const common) {
	curl_multi_remove_handle(common->multi_handle, common->easy_handle);
	string_free(common->base_url, &common->base_url_len);
	string_free(common->parameters, &common->parameters_len);
	string_free(common->complete_url, &common->complete_url_len);
	string_free(common->auth_token, &common->auth_token_len);
	string_free(common->out_file, &common->out_file_len);
	ctw_clear_headers(common);
	curl_global_cleanup();
#ifdef NEEDS_CERT_PATH
	string_free(common->cert_path, &common->cert_path_len);
#endif 
}

#ifdef NEEDS_CERT_PATH
static void ctw_set_cert_path(struct _ctw *const common, const char *const directory) {
	common->cert_path = string_create(&common->cert_path_len, strlen(directory) + 11);
	strcpy(common->cert_path, directory);
	strcat(common->cert_path, "/cacert.pem");
}
#endif 
