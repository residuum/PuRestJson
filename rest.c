/*
 * [rest] makes RESTful calls to webservices.
 * */

#include "purest_json.h"

#include "curl_thread_wrapper.c"
#include "shared_functions.c"

static t_class *rest_class;

static void *get_cookie_auth_token(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	struct _memory_struct out_content;
	struct _memory_struct out_header;
	char *post_data;
	size_t post_data_length;
	char *header_line;
	char *cookie_params;
	long http_code;
	t_atom auth_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		memset(x->threaddata.complete_url, 0x00, MAXPDSTRING);
		post_data_length = strlen(x->cookie.username) + strlen(x->cookie.password) + 17;/*name=&password=*/
		post_data = (char *)getbytes(sizeof(char) * post_data_length);
		if (post_data == NULL) {
			error("not enough memory");
		} else {
			strcpy(post_data, "name=");
			strcat(post_data, x->cookie.username);
			strcat(post_data, "&password=");
			strcat(post_data, x->cookie.password);
		}
		strcpy(x->threaddata.complete_url, x->threaddata.base_url);
		strcat(x->threaddata.complete_url, x->cookie.login_path);
		curl_easy_setopt(curl_handle, CURLOPT_URL, x->threaddata.complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
		out_content.memory = getbytes(1);
		out_content.size = 0;
		out_header.memory = getbytes(1);
		out_header.size = 0;
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

		if (result == CURLE_OK) {
			if (out_header.memory) {
				header_line = strtok(out_header.memory, "\n");
				while (header_line != NULL) {
					if (strncmp(header_line, "Set-Cookie:", 11) == 0) {
						cookie_params = strtok(header_line, ": ");
						/*remove "Set-Cookie:" */
						cookie_params = strtok(NULL, "; ");
						while (cookie_params != NULL) {
							if (strlen(cookie_params) != 0) {
								strcpy(x->cookie.auth_token, cookie_params);
								break;
							}
							cookie_params = strtok(NULL, "; ");
						}
						break;
					}
					header_line = strtok(NULL, "\n");
				}

				/* Free memory */
				freebytes(out_header.memory, (out_header.size + 1) * sizeof(char));
			}
			if (out_content.memory) {
				freebytes(out_content.memory, (out_content.size + 1) * sizeof(char));
			}
		}
		freebytes(post_data, post_data_length * sizeof(char));
	}
	x->threaddata.is_data_locked = 0;
	return NULL;
}

static void set_url_parameters(t_rest *x, int argcount, t_atom *argvec) {
	switch (argcount) {
		case 0:
			memset(x->threaddata.base_url, 0x00, MAXPDSTRING);
			memset(x->cookie.login_path, 0x00, MAXPDSTRING);
			memset(x->cookie.username, 0x00, MAXPDSTRING);
			memset(x->cookie.password, 0x00, MAXPDSTRING);
			memset(x->cookie.auth_token, 0x00, MAXPDSTRING);
			break;
		case 1:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->threaddata.base_url, MAXPDSTRING);
			}
			memset(x->cookie.login_path, 0x00, MAXPDSTRING);
			memset(x->cookie.username, 0x00, MAXPDSTRING);
			memset(x->cookie.password, 0x00, MAXPDSTRING);
			memset(x->cookie.auth_token, 0x00, MAXPDSTRING);
			break;
		case 4:
			x->threaddata.is_data_locked = 1;
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->threaddata.base_url, MAXPDSTRING);
			}
			if (argvec[1].a_type != A_SYMBOL) {
				error("Username cannot be set.");
			} else {
				atom_string(argvec + 1, x->cookie.login_path, MAXPDSTRING);
			}
			if (argvec[2].a_type != A_SYMBOL) {
				error("Username cannot be set.");
			} else {
				atom_string(argvec + 2, x->cookie.username, MAXPDSTRING);
			}
			if (argvec[3].a_type != A_SYMBOL) {
				error("Password cannot be set.");
			} else {
				atom_string(argvec + 3, x->cookie.password, MAXPDSTRING);
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
			0, sizeof(t_rest), 0, A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_url, gensym("url"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("PUT"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("DELETE"), A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_command, gensym("POST"), A_GIMME, 0);
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
		memset(x->threaddata.parameters, 0x00, MAXPDSTRING);
		memset(x->threaddata.complete_url, 0x00, MAXPDSTRING);
		switch (argcount) {
			case 0:
				break;
			default:
				request_type = selector->s_name;
				atom_string(argvec, path, MAXPDSTRING);
				x->threaddata.is_data_locked = 1;
				if (argcount > 1) {
					atom_string(argvec + 1, parameters, MAXPDSTRING);
					if (parameters != NULL) {
						cleaned_parameters = remove_backslashes(parameters, memsize);
						strcpy(x->threaddata.parameters, cleaned_parameters);
						freebytes(cleaned_parameters, memsize);
					}
				}
				if (x->threaddata.base_url != NULL) {
					strcpy(x->threaddata.complete_url, x->threaddata.base_url);
				}
				strcat(x->threaddata.complete_url, path);
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

void *rest_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_rest *x = (t_rest *)pd_new(rest_class);

	(void) selector;

	set_url_parameters(x, 0, argvec); 
	set_url_parameters(x, argcount, argvec); 

	outlet_new(&x->threaddata.x_ob, NULL);
	x->threaddata.status_info_outlet = outlet_new(&x->threaddata.x_ob, NULL);
	x->threaddata.is_data_locked = 0;

	return (void *)x;
}
