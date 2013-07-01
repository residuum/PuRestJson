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

static void *get_cookie_auth_token(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	struct _memory_struct out_content;
	struct _memory_struct out_header;
	char *post_data;
	size_t post_data_len;
	char *header_line;
	char *cookie_params;
	long http_code;
	t_atom auth_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (!curl_handle) {
		x->common.locked = 0;
		return NULL;
	}

	/* length + name=&password=*/
	post_data = string_create(&post_data_len, x->cookie.username_len + x->cookie.password_len + 17);
	if (post_data == NULL) {
		MYERROR("not enough memory");
	} else {
		strcpy(post_data, "name=");
		strcat(post_data, x->cookie.username);
		strcat(post_data, "&password=");
		strcat(post_data, x->cookie.password);
	}
	x->common.complete_url = string_create(&x->common.complete_url_len, 
			x->common.base_url_len + x->cookie.login_path_len);
	strcpy(x->common.complete_url, x->common.base_url);
	strcat(x->common.complete_url, x->cookie.login_path);
	curl_easy_setopt(curl_handle, CURLOPT_URL, x->common.complete_url);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
	out_content.memory = getbytes(1);
	out_content.size = 0;
	out_header.memory = getbytes(1);
	out_header.size = 0;
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, ctw_write_mem_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_content);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, ctw_write_mem_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *)&out_header);
	result = curl_easy_perform(curl_handle);

	/* output the status code at second outlet */
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
	SETSYMBOL(&auth_status_data[0], gensym("cookie"));
	if (http_code == 200 && result == CURLE_OK) {
		SETSYMBOL(&auth_status_data[1], gensym("bang"));
		outlet_list(x->common.stat_out, &s_list, 2, &auth_status_data[0]);
	} else {
		SETFLOAT(&auth_status_data[1], (float)http_code);
		SETFLOAT(&auth_status_data[2], (float)result);
		outlet_list(x->common.stat_out, &s_list, 3, &auth_status_data[0]);
	}

	string_free(x->common.auth_token, &x->common.auth_token_len);
	if (result == CURLE_OK) {
		if (out_header.memory) {
			header_line = strtok(out_header.memory, "\n");
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
			string_free(out_header.memory, &out_header.size);
		}
		string_free(out_content.memory, &out_content.size);
	}
	string_free(post_data, &post_data_len);
	x->common.locked = 0;
	return NULL;
}

static void set_url_parameters(t_rest *x, int argc, t_atom *argv) {
	char temp[MAXPDSTRING];

	rest_free_inner(x);
	switch (argc) {
		case 0:
			break;
		case 1:
			if (argv[0].a_type != A_SYMBOL) {
				pd_error(x, "Base URL cannot be set.");
			} else {
				atom_string(argv, temp, MAXPDSTRING);
				x->common.base_url = string_create(&x->common.base_url_len, strlen(temp));
				strcpy(x->common.base_url, temp);
			}
			break;
		case 4:
			x->common.locked = 1;
			if (argv[0].a_type != A_SYMBOL) {
				pd_error(x, "Base URL cannot be set.");
			} else {
				atom_string(argv, temp, MAXPDSTRING);
				x->common.base_url = string_create(&x->common.base_url_len, strlen(temp));
				strcpy(x->common.base_url, temp);
			}
			if (argv[1].a_type != A_SYMBOL) {
				pd_error(x, "Cookie path cannot be set.");
			} else {
				atom_string(argv + 1, temp, MAXPDSTRING);
				x->cookie.login_path = string_create(&x->cookie.login_path_len, strlen(temp));
				strcpy(x->cookie.login_path, temp);
			}
			if (argv[2].a_type != A_SYMBOL) {
				pd_error(x, "Username cannot be set.");
			} else {
				atom_string(argv + 2, temp, MAXPDSTRING);
				x->cookie.username = string_create(&x->cookie.username_len, strlen(temp));
				strcpy(x->cookie.username, temp);
			}
			if (argv[3].a_type != A_SYMBOL) {
				pd_error(x, "Password cannot be set.");
			} else {
				atom_string(argv + 3, temp, MAXPDSTRING);
				x->cookie.password = string_create(&x->cookie.password_len, strlen(temp));
				strcpy(x->cookie.password, temp);
			}
			ctw_thread_exec((void *)x, get_cookie_auth_token);
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
}

void rest_command(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {
	char *req_type;
	char path[MAXPDSTRING];
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;
	t_atom auth_status_data[2];

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
		SETSYMBOL(&auth_status_data[0], gensym("request"));
		SETSYMBOL(&auth_status_data[1], gensym("Request method not supported"));
		pd_error(x, "Request method %s not supported.", x->common.req_type);
		outlet_list(x->common.stat_out, &s_list, 2, &auth_status_data[0]);
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
	ctw_thread_exec((void *)x, ctw_exec_req);
}

void rest_url(t_rest *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("rest object is performing request and locked");
	} else {
		set_url_parameters(x, argc, argv); 
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

void *rest_new(t_symbol *sel, int argc, t_atom *argv) {
	t_rest *x = (t_rest *)pd_new(rest_class);

	(void) sel;

	ctw_init((struct _ctw *)x);
	ctw_set_timeout((struct _ctw *)x, 0);

	set_url_parameters(x, 0, argv); 
	set_url_parameters(x, argc, argv); 

	outlet_new(&x->common.x_ob, NULL);
	x->common.stat_out = outlet_new(&x->common.x_ob, NULL);
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
