/*
 * [rest] makes RESTful calls to webservices.
 * */

#include "purest_json.h"

#include "curl_thread_wrapper.c"
#include "shared_functions.c"

static t_class *rest_class;

static void rest_free_inner(t_rest *x) {
	rest_common_free((struct _rest_common *)x);
	free_string(x->cookie.login_path, &x->cookie.login_path_len);
	free_string(x->cookie.username, &x->cookie.username_len);
	free_string(x->cookie.password, &x->cookie.password_len);
	free_string(x->cookie.auth_token, &x->cookie.auth_token_len);
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
	if (curl_handle) {
		/* length + name=&password=*/
		post_data = get_string(&post_data_len, x->cookie.username_len + x->cookie.password_len + 17);
		if (post_data == NULL) {
			error("not enough memory");
		} else {
			strcpy(post_data, "name=");
			strcat(post_data, x->cookie.username);
			strcat(post_data, "&password=");
			strcat(post_data, x->cookie.password);
		}
		x->threaddata.complete_url = get_string(&x->threaddata.complete_url_len, 
				x->threaddata.base_url_len + x->cookie.login_path_len);
		strcpy(x->threaddata.complete_url, x->threaddata.base_url);
		strcat(x->threaddata.complete_url, x->cookie.login_path);
		curl_easy_setopt(curl_handle, CURLOPT_URL, x->threaddata.complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
		out_content.memory = get_string(&out_content.size, 0);
		out_header.memory = get_string(&out_header.size, 0);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_content);
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *)&out_header);
		result = curl_easy_perform(curl_handle);

		/* output the status code at second outlet */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
		SETSYMBOL(&auth_status_data[0], gensym("cookie"));
		if (http_code == 200 && result == CURLE_OK) {
			SETSYMBOL(&auth_status_data[1], gensym("bang"));
			outlet_list(x->threaddata.status_info_outlet, &s_list, 2, &auth_status_data[0]);
		} else {
			SETFLOAT(&auth_status_data[1], (float)http_code);
			SETFLOAT(&auth_status_data[2], (float)result);
			outlet_list(x->threaddata.status_info_outlet, &s_list, 3, &auth_status_data[0]);
		}

		free_string(x->cookie.auth_token, &x->cookie.auth_token_len);
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
								x->cookie.auth_token = get_string(&x->cookie.auth_token_len, strlen(cookie_params));
								strcpy(x->cookie.auth_token, cookie_params);
								break;
							}
							cookie_params = strtok(NULL, "; ");
						}
						break;
					}
					header_line = strtok(NULL, "\n");
				}
				free_string(out_header.memory, &out_header.size);
			}
			free_string(out_content.memory, &out_content.size);
		}
		free_string(post_data, &post_data_len);
	}
	x->threaddata.is_data_locked = 0;
	return NULL;
}

static void set_url_parameters(t_rest *x, int argcount, t_atom *argvec) {
	char temp[MAXPDSTRING];

	rest_free_inner(x);
	switch (argcount) {
		case 0:
			break;
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, temp, MAXPDSTRING);
				x->threaddata.base_url = get_string(&x->threaddata.base_url_len, strlen(temp));
				strcpy(x->threaddata.base_url, temp);
			}
			break;
		case 4:
			x->threaddata.is_data_locked = 1;
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, temp, MAXPDSTRING);
				x->threaddata.base_url = get_string(&x->threaddata.base_url_len, strlen(temp));
				strcpy(x->threaddata.base_url, temp);
			}
			if (argvec[1].a_type != A_SYMBOL) {
				error("Cookie path cannot be set.");
			} else {
				atom_string(argvec + 1, temp, MAXPDSTRING);
				x->cookie.login_path = get_string(&x->cookie.login_path_len, strlen(temp));
				strcpy(x->cookie.login_path, temp);
			}
			if (argvec[2].a_type != A_SYMBOL) {
				error("Username cannot be set.");
			} else {
				atom_string(argvec + 2, temp, MAXPDSTRING);
				x->cookie.username = get_string(&x->cookie.username_len, strlen(temp));
				strcpy(x->cookie.username, temp);
			}
			if (argvec[3].a_type != A_SYMBOL) {
				error("Password cannot be set.");
			} else {
				atom_string(argvec + 3, temp, MAXPDSTRING);
				x->cookie.password = get_string(&x->cookie.password_len, strlen(temp));
				strcpy(x->cookie.password, temp);
			}
			thread_execute((struct _rest_common *)x, get_cookie_auth_token);
			break;
		default:
			error("Wrong number of parameters.");
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
}

void rest_command(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *request_type;
	char path[MAXPDSTRING];
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;
	t_atom auth_status_data[2];

	if(x->threaddata.is_data_locked) {
		post("rest object is performing request and locked");
	} else {
		memset(x->threaddata.request_type, 0x00, REQUEST_TYPE_LEN);
		switch (argcount) {
			case 0:
				break;
			default:
				x->threaddata.is_data_locked = 1;
				request_type = selector->s_name;
				strcpy(x->threaddata.request_type, request_type);
				if ((strcmp(x->threaddata.request_type, "GET") && 
							strcmp(x->threaddata.request_type, "POST") && 
							strcmp(x->threaddata.request_type, "PUT") &&
							strcmp(x->threaddata.request_type, "DELETE"))) {
					SETSYMBOL(&auth_status_data[0], gensym("request"));
					SETSYMBOL(&auth_status_data[1], gensym("Request method not supported"));
					error("Request method %s not supported.", x->threaddata.request_type);
					outlet_list(x->threaddata.status_info_outlet, &s_list, 2, &auth_status_data[0]);
					x->threaddata.is_data_locked = 0;
				} else {
					atom_string(argvec, path, MAXPDSTRING);
					x->threaddata.complete_url = get_string(&x->threaddata.complete_url_len,
							x->threaddata.base_url_len + strlen(path) + 1);
					if (x->threaddata.base_url != NULL) {
						strcpy(x->threaddata.complete_url, x->threaddata.base_url);
					}
					strcat(x->threaddata.complete_url, path);
					if (argcount > 1) {
						atom_string(argvec + 1, parameters, MAXPDSTRING);
						if (strlen(parameters)) {
							cleaned_parameters = remove_backslashes(parameters, &memsize);
							x->threaddata.parameters = get_string(&x->threaddata.parameters_len, memsize + 1);
							strcpy(x->threaddata.parameters, cleaned_parameters);
							freebytes(cleaned_parameters, memsize);
						}
					}
					thread_execute((struct _rest_common *)x, execute_request);
				}
				break;
		}
	}
}

void rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {

	(void) selector;

	if(x->threaddata.is_data_locked) {
		post("rest object is performing request and locked");
	} else {
		set_url_parameters(x, argcount, argvec); 
	}
}

void rest_timeout(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {

	(void) selector;

	if(x->threaddata.is_data_locked) {
		post("rest object is performing request and locked");
	} else if (argcount > 2){
		error("timeout must have 0 or 1 parameter");
	} else if (argcount == 0) {
		set_timeout((struct _rest_common *)x, 0);
	} else {
		set_timeout((struct _rest_common *)x, atom_getint(argvec));
	}
}

void *rest_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_rest *x = (t_rest *)pd_new(rest_class);

	(void) selector;

	set_timeout((struct _rest_common *)x, 0);

	x->threaddata.base_url_len = 0;
	x->threaddata.parameters_len = 0;
	x->threaddata.complete_url_len = 0;
	x->threaddata.parameters_len = 0;

	set_url_parameters(x, 0, argvec); 
	set_url_parameters(x, argcount, argvec); 

	outlet_new(&x->threaddata.x_ob, NULL);
	x->threaddata.status_info_outlet = outlet_new(&x->threaddata.x_ob, NULL);
	x->threaddata.is_data_locked = 0;

	return (void *)x;
}

void rest_free(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	rest_free_inner(x);
}
