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

/*
 * [rest] makes RESTful calls to webservices.
 * */

#include "rest.h"

#include "inc/string.c"
#include "inc/strlist.c"
#include "inc/ctw.c"

struct _rest {
	struct _ctw common;
	/* authentication: cookie */
	struct {
		size_t login_path_len;
		char *login_path;
		size_t username_len;
		char *username;
		size_t password_len;
		char *password;
	} cookie;
};

static t_class *rest_class;

/* frees data */
static void rest_free_inner(t_rest *rest);
/* extracts cookie data */
static void rest_extract_token(t_rest *rest, struct _memory_struct *out_header);
/* processes output of login */
static void rest_process_auth_data(t_rest *rest, struct _memory_struct *out_header);
/* performs an HTTP POST for logging in */
static void *rest_get_auth_token(void *thread_args);
/* initialises object and optionally performs login */
static void rest_set_init(t_rest *rest, const int argc, t_atom *argv);

/* begin implementations */
static void rest_free_inner(t_rest *const rest) {
	ctw_free((struct _ctw *)rest);
	string_free(rest->cookie.login_path, &rest->cookie.login_path_len);
	string_free(rest->cookie.username, &rest->cookie.username_len);
	string_free(rest->cookie.password, &rest->cookie.password_len);
}

static void rest_extract_token(t_rest *const rest, struct _memory_struct *const out_header) {
	if ((*out_header).memory) {
		char *cookie_params = NULL;
		char *header_line = strtok((*out_header).memory, "\n");
		while (header_line != NULL) {
			if (strncmp(header_line, "Set-Cookie:", 11) == 0) {
				cookie_params = strtok(header_line, ": ");
				/*remove "Set-Cookie:" */
				cookie_params = strtok(NULL, "; ");
				while (cookie_params != NULL) {
					if (strlen(cookie_params)) {
						rest->common.auth_token = string_create(
								&rest->common.auth_token_len, 
								strlen(cookie_params));
						strcpy(rest->common.auth_token, cookie_params);
						break;
					}
					cookie_params = strtok(NULL, "; ");
				}
				break;
			}
			header_line = strtok(NULL, "\n");
		}
	}
}

static void rest_process_auth_data(t_rest *const rest, struct _memory_struct *const out_header) {
	CURLMsg *msg;
	int msgs_left;

	while ((msg = curl_multi_info_read(rest->common.multi_handle, &msgs_left))) {
		if (msg->msg == CURLMSG_DONE) {
			long http_status;
			/* output status */
			curl_easy_getinfo(rest->common.easy_handle, CURLINFO_RESPONSE_CODE, &http_status);
			if (http_status >= 200 && http_status < 300) {
				outlet_bang(rest->common.status_out);
				if (msg->data.result == CURLE_OK) {
					rest_extract_token(rest, out_header);
				} else {
					t_atom http_status_data[2];
					SETFLOAT(&http_status_data[0], (float)http_status);
					SETSYMBOL(&http_status_data[1], 
							gensym(curl_easy_strerror(msg->data.result)));
					pd_error(rest, "Error while performing request: %s.", 
							curl_easy_strerror(msg->data.result));
					outlet_list(rest->common.status_out, &s_list, 2, &http_status_data[0]);
				}
			}
			curl_easy_cleanup(rest->common.easy_handle);
			curl_multi_cleanup(rest->common.multi_handle);
		}
	}
}

static void *rest_get_auth_token(void *const thread_args) {
	t_rest *const rest = thread_args;

	/* length + name=&password=*/
	rest->common.parameters = string_create(&rest->common.parameters_len, 
			rest->cookie.username_len + rest->cookie.password_len + 17);
	if (rest->common.parameters != NULL) {
		strcpy(rest->common.parameters, "name=");
		strcat(rest->common.parameters, rest->cookie.username);
		strcat(rest->common.parameters, "&password=");
		strcat(rest->common.parameters, rest->cookie.password);
	}
	rest->common.complete_url = string_create(&rest->common.complete_url_len, 
			rest->common.base_url_len + rest->cookie.login_path_len);
	strcpy(rest->common.complete_url, rest->common.base_url);
	strcat(rest->common.complete_url, rest->cookie.login_path);
	strcpy(rest->common.req_type, "POST");

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	rest->common.easy_handle = curl_easy_init();
	rest->common.multi_handle = curl_multi_init();
	if (rest->common.easy_handle == NULL) {
		MYERROR("Cannot init curl.");
		ctw_cleanup_request(&rest->common, NULL, NULL);
	} else {
		struct curl_slist *slist = NULL;
		struct _memory_struct out_content;
		struct _memory_struct out_header;
		FILE *fp = ctw_prepare(&rest->common, slist, &out_content, NULL);

		out_header.memory = getbytes(1);
		out_header.size = 0;

		struct _cb_val *cb_val = getbytes(sizeof(struct _cb_val));
		cb_val->mem = &out_header;
		cb_val->ctw = (struct _ctw *)rest;
		curl_easy_setopt(rest->common.easy_handle, CURLOPT_HEADERFUNCTION, ctw_write_mem_cb);
		curl_easy_setopt(rest->common.easy_handle, CURLOPT_WRITEHEADER, (void *)cb_val);
		ctw_thread_perform(&rest->common);
		rest_process_auth_data(rest, &out_header);
		string_free(out_header.memory, &out_header.size);
		string_free(out_content.memory, &out_content.size);
		ctw_cleanup_request(&rest->common, fp, slist);
	}
	return NULL;
}

static void rest_set_init(t_rest *const rest, const int argc, t_atom *const argv) {
	rest_free_inner(rest);

	switch (argc) {
		case 0:
			break;
		case 1:
			rest->common.base_url = ctw_set_param((struct _ctw *)rest, argv, &rest->common.base_url_len, 
					"Base URL cannot be set.");
			break;
		case 4:
			rest->common.locked = 1;
			rest->common.base_url = ctw_set_param((struct _ctw *)rest, argv, &rest->common.base_url_len, 
					"Base URL cannot be set.");
			rest->cookie.login_path = ctw_set_param((struct _ctw *)rest, argv + 1, 
					&rest->cookie.login_path_len, "Cookie path cannot be set.");
			rest->cookie.username = ctw_set_param((struct _ctw *)rest, argv + 2, &rest->cookie.username_len,
					"Username cannot be set.");
			rest->cookie.password = ctw_set_param((struct _ctw *)rest, argv + 3, &rest->cookie.password_len, 
					"Password cannot be set.");
			string_free(rest->common.auth_token, &rest->common.auth_token_len);
			ctw_thread_exec((void *)rest, rest_get_auth_token);
			break;
		default:
			pd_error(rest, "Wrong number of parameters.");
			break;
	}
}

void rest_setup(void) {
	rest_class = class_new(gensym("rest"), (t_newmethod)rest_new,
			(t_method)rest_free, sizeof(t_rest), 0, A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_init, gensym("init"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("POST"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("HEAD"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("PUT"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("DELETE"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("PATCH"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("OPTIONS"), A_GIMME, 0);
	/*class_addmethod(rest_class, (t_method)rest_command, gensym("CONNECT"), A_GIMME, 0);*/
	class_addmethod(rest_class, (t_method)rest_command, gensym("TRACE"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_timeout, gensym("timeout"), A_DEFFLOAT, 0);
	class_addmethod(rest_class, (t_method)rest_sslcheck, gensym("sslcheck"), A_DEFFLOAT, 0);
	class_addmethod(rest_class, (t_method)rest_cancel, gensym("cancel"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_header, gensym("header"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_clear_headers, gensym("header_clear"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_file, gensym("file"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_mode, gensym("mode"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_proxy, gensym("proxy"), A_GIMME, 0);
}

void rest_command(t_rest *const rest, const t_symbol *const sel, const int argc, t_atom *argv) {
	const char *const req_type = sel->s_name;
	char path[MAXPDSTRING];

	if (rest->common.locked) {
		post("rest object is performing request and locked.");
		return;
	}

	memset(rest->common.req_type, 0x00, REQUEST_TYPE_LEN);
	if (argc == 0) {
		return;
	}

	rest->common.locked = 1;
	strncpy(rest->common.req_type, req_type, REQUEST_TYPE_LEN - 1);
	if (ctw_check_request_type(rest->common.req_type) != 0){
		pd_error(rest, "Request method %s not supported.", rest->common.req_type);
		rest->common.locked = 0;
		return;
	}
	atom_string(argv, path, MAXPDSTRING);
	rest->common.complete_url = string_create(&rest->common.complete_url_len,
			rest->common.base_url_len + strlen(path) + 1);
	if (rest->common.base_url != NULL) {
		strcpy(rest->common.complete_url, rest->common.base_url);
	}
	strcat(rest->common.complete_url, path);
	if (argc > 1) {
		char parameters[MAXPDSTRING];
		atom_string(argv + 1, parameters, MAXPDSTRING);
		if (strlen(parameters)) {
			char *cleaned_parameters;
			size_t memsize = 0;

			cleaned_parameters = string_remove_backslashes(parameters, &memsize);
			rest->common.parameters = string_create(&rest->common.parameters_len, memsize + 1);
			strcpy(rest->common.parameters, cleaned_parameters);
			freebytes(cleaned_parameters, memsize);
		}
	}
	ctw_thread_exec((struct _ctw *)rest, ctw_exec);
}

void rest_init(t_rest *const rest, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	if (rest->common.locked) {
		post("rest object is performing request and locked.");
	} else {
		rest_set_init(rest, argc, argv); 
	}
}

void rest_timeout(t_rest *const rest, const t_floatarg f) {
	if (rest->common.locked) {
		post("rest object is performing request and locked.");
	} else {
		ctw_set_timeout((struct _ctw *)rest, (int)f);
	}
}

void rest_sslcheck(t_rest *const rest, const t_floatarg f) {
	if (rest->common.locked) {
		post("rest object is performing request and locked.");
	} else {
		ctw_set_sslcheck((struct _ctw *)rest, (int)f);
	}
}

void rest_cancel(t_rest *const rest, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_cancel((struct _ctw *)rest);
}

void rest_header(t_rest *const rest, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_add_header((struct _ctw *)rest, argc, argv);
}

void rest_clear_headers(t_rest *const rest, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_clear_headers((struct _ctw *)rest);
}

void rest_file(t_rest *const rest, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_set_file((struct _ctw *)rest, argc, argv);
}

void rest_mode(t_rest *const rest, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_set_mode((struct _ctw *)rest, argc, argv);
}

void rest_proxy(t_rest *const rest, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_set_proxy((struct _ctw *)rest, argc, argv);
}

void *rest_new(t_symbol *const sel, const int argc, t_atom *const argv) {
	t_rest *const rest = (t_rest *)pd_new(rest_class);

	(void) sel;

	ctw_init((struct _ctw *)rest);
	ctw_set_timeout((struct _ctw *)rest, 0);

	rest_set_init(rest, 0, argv); 
	rest_set_init(rest, argc, argv); 

	outlet_new(&rest->common.x_ob, NULL);
	rest->common.status_out = outlet_new(&rest->common.x_ob, NULL);
	rest->common.locked = 0;
#ifdef NEEDS_CERT_PATH
	ctw_set_cert_path((struct _ctw *)rest, rest_class->c_externdir->s_name);
#endif
	purest_json_lib_info("rest");
	return (void *)rest;
}

void rest_free(t_rest *const rest, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	rest_free_inner(rest);
}
