/*
 * [rest] makes RESTful calls to webservices.
 * */

#include "rest.h"

#include "string.c"
#include "strlist.c"
#include "ctw.c"

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

static void rest_free_inner(t_rest *x) {
	ctw_free((struct _ctw *)x);
	string_free(x->cookie.login_path, &x->cookie.login_path_len);
	string_free(x->cookie.username, &x->cookie.username_len);
	string_free(x->cookie.password, &x->cookie.password_len);
}

static void rest_extract_token(t_rest *x, struct _memory_struct *out_header) {
	CURLMsg *msg;
	int msgs_left;
	long http_status;
	t_atom http_status_data[3];
	char *header_line;
	char *cookie_params;

	while ((msg = curl_multi_info_read(x->common.multi_handle, &msgs_left))) {
		if (msg->msg == CURLMSG_DONE) {
			/* output status */
			curl_easy_getinfo(x->common.easy_handle, CURLINFO_RESPONSE_CODE, &http_status);
			SETSYMBOL(&http_status_data[0], gensym("cookie"));
			if (http_status >= 200 && http_status < 300) {
				SETSYMBOL(&http_status_data[1], gensym("bang"));
				outlet_list(x->common.status_out, &s_list, 2, &http_status_data[0]);
				if (msg->data.result == CURLE_OK) {
					string_free(x->common.auth_token, &x->common.auth_token_len);
					if ((*out_header).memory) {
						header_line = strtok((*out_header).memory, "\n");
						while (header_line != NULL) {
							if (strncmp(header_line, "Set-Cookie:", 11) == 0) {
								cookie_params = strtok(header_line, ": ");
								/*remove "Set-Cookie:" */
								cookie_params = strtok(NULL, "; ");
								while (cookie_params != NULL) {
									if (strlen(cookie_params)) {
										x->common.auth_token = string_create(&x->common.auth_token_len, strlen(cookie_params));
										strcpy(x->common.auth_token, cookie_params);
										break;
									}
									cookie_params = strtok(NULL, "; ");
								}
								break;
							}
							header_line = strtok(NULL, "\n");
						}
					}
				} else {
					SETFLOAT(&http_status_data[1], (float)http_status);
					SETSYMBOL(&http_status_data[2], gensym(curl_easy_strerror(msg->data.result)));
					pd_error(x, "Error while performing request: %s", curl_easy_strerror(msg->data.result));
					outlet_list(x->common.status_out, &s_list, 3, &http_status_data[0]);
				}
			}
			curl_easy_cleanup(x->common.easy_handle);
			curl_multi_cleanup(x->common.multi_handle);
		}
	}
}

static void *rest_get_auth_token(void *thread_args) {
	t_rest *x = thread_args; 
	struct curl_slist *slist = NULL;
	struct _memory_struct out_content;
	struct _memory_struct out_header;
	FILE *fp; 

	/* length + name=&password=*/
	x->common.parameters = string_create(&x->common.parameters_len, x->cookie.username_len + x->cookie.password_len + 17);
	if (x->common.parameters != NULL) {
		strcpy(x->common.parameters, "name=");
		strcat(x->common.parameters, x->cookie.username);
		strcat(x->common.parameters, "&password=");
		strcat(x->common.parameters, x->cookie.password);
	}
	x->common.complete_url = string_create(&x->common.complete_url_len, 
			x->common.base_url_len + x->cookie.login_path_len);
	strcpy(x->common.complete_url, x->common.base_url);
	strcat(x->common.complete_url, x->cookie.login_path);
	strcpy(x->common.req_type, "POST");

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	x->common.easy_handle = curl_easy_init();
	x->common.multi_handle = curl_multi_init();
	if (!x->common.easy_handle) {
		MYERROR("Cannot init curl.");
	} else {
		fp = ctw_prepare(&x->common, slist, &out_content);
		out_header.memory = getbytes(1);
		out_header.size = 0;
		curl_easy_setopt(x->common.easy_handle, CURLOPT_HEADERFUNCTION, ctw_write_mem_cb);
		curl_easy_setopt(x->common.easy_handle, CURLOPT_WRITEHEADER, (void *)&out_header);
		ctw_thread_perform(&x->common);
		rest_extract_token(x, &out_header);
		string_free(out_header.memory, &out_header.size);
		string_free(out_content.memory, &out_content.size);
		string_free(x->common.complete_url, &x->common.complete_url_len);
		string_free(x->common.parameters, &x->common.parameters_len);
		if (slist != NULL) {
			curl_slist_free_all(slist);
		}
		if (fp) {
			fclose(fp);
		}
	}
	x->common.locked = 0;
	return NULL;
}

static void rest_set_url_params(t_rest *x, int argc, t_atom *argv) {
	rest_free_inner(x);

	switch (argc) {
		case 0:
			break;
		case 1:
			x->common.base_url = ctw_set_param((void *)x, argv, &x->common.base_url_len, "Base URL cannot be set.");
			break;
		case 4:
			x->common.locked = 1;
			x->common.base_url = ctw_set_param((void *)x, argv, &x->common.base_url_len, "Base URL cannot be set.");
			x->cookie.login_path = ctw_set_param((void *)x, argv + 1, &x->cookie.login_path_len, "Cookie path cannot be set.");
			x->cookie.username = ctw_set_param((void *)x, argv + 2, &x->cookie.username_len, "Username cannot be set.");
			x->cookie.password = ctw_set_param((void *)x, argv + 3, &x->cookie.password_len, "Password cannot be set.");
			ctw_thread_exec((void *)x, rest_get_auth_token);
			break;
		default:
			pd_error(x, "Wrong number of parameters.");
			break;
	}
}

void rest_setup(void) {
	rest_class = class_new(gensym("rest"), (t_newmethod)rest_new,
			(t_method)rest_free, sizeof(t_rest), 0, A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_url, gensym("url"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("PUT"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("DELETE"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("POST"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_timeout, gensym("timeout"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_sslcheck, gensym("sslcheck"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_cancel, gensym("cancel"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_header, gensym("header"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_clear_headers, gensym("header_clear"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_write, gensym("write"), A_GIMME, 0);
}

void rest_command(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {
	char *req_type;
	char path[MAXPDSTRING];
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;

	if(x->common.locked) {
		post("rest object is performing request and locked");
		return;
	}

	memset(x->common.req_type, 0x00, REQUEST_TYPE_LEN);
	if (argc == 0) {
		return;
	}

	x->common.locked = 1;
	req_type = sel->s_name;
	strcpy(x->common.req_type, req_type);
	if ((strcmp(x->common.req_type, "GET") && 
				strcmp(x->common.req_type, "POST") && 
				strcmp(x->common.req_type, "PUT") &&
				strcmp(x->common.req_type, "DELETE"))) {
		pd_error(x, "Request method %s not supported.", x->common.req_type);
		x->common.locked = 0;
		return;
	} 

	atom_string(argv, path, MAXPDSTRING);
	x->common.complete_url = string_create(&x->common.complete_url_len,
			x->common.base_url_len + strlen(path) + 1);
	if (x->common.base_url != NULL) {
		strcpy(x->common.complete_url, x->common.base_url);
	}
	strcat(x->common.complete_url, path);
	if (argc > 1) {
		atom_string(argv + 1, parameters, MAXPDSTRING);
		if (strlen(parameters)) {
			cleaned_parameters = string_remove_backslashes(parameters, &memsize);
			x->common.parameters = string_create(&x->common.parameters_len, memsize + 1);
			strcpy(x->common.parameters, cleaned_parameters);
			freebytes(cleaned_parameters, memsize);
		}
	}
	ctw_thread_exec((void *)x, ctw_exec);
}

void rest_url(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("rest object is performing request and locked");
	} else {
		rest_set_url_params(x, argc, argv); 
	}
}

void rest_timeout(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("rest object is performing request and locked");
	} else if (argc > 2){
		pd_error(x, "timeout must have 0 or 1 parameter");
	} else if (argc == 0) {
		ctw_set_timeout((struct _ctw *)x, 0);
	} else {
		ctw_set_timeout((struct _ctw *)x, atom_getint(argv));
	}
}

void rest_sslcheck(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("rest object is performing request and locked");
	} else if (argc != 1){
		pd_error(x, "sslcheck must have 1 parameter");
	} else {
		ctw_set_sslcheck((struct _ctw *)x, atom_getint(argv));
	}
}

void rest_cancel(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_cancel((struct _ctw *)x);
}

void rest_header(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	ctw_add_header((void *)x, argc, argv);
}

void rest_clear_headers(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_clear_headers((struct _ctw *)x);
}

void rest_write(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	ctw_set_file((void *)x, argc, argv);
}

void *rest_new(t_symbol *sel, int argc, t_atom *argv) {
	t_rest *x = (t_rest *)pd_new(rest_class);

	(void) sel;

	ctw_init((struct _ctw *)x);
	ctw_set_timeout((struct _ctw *)x, 0);

	rest_set_url_params(x, 0, argv); 
	rest_set_url_params(x, argc, argv); 

	outlet_new(&x->common.x_ob, NULL);
	x->common.status_out = outlet_new(&x->common.x_ob, NULL);
	x->common.locked = 0;
#ifdef NEEDS_CERT_PATH
	ctw_set_cert_path((struct _ctw *)x, rest_class->c_externdir->s_name);
#endif
	return (void *)x;
}

void rest_free(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	rest_free_inner(x);
}
