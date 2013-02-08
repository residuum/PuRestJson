/*
 * [oauth] communicates with OAUTH webservices via GET and POST.
 * */

#include "purest_json.h"

#include "curl_thread_wrapper.c"
#include "shared_functions.c"

static t_class *oauth_class;

static void set_url_parameters(t_oauth *x, int argcount, t_atom *argvec) {
	switch (argcount) {
		case 0:
			memset(x->threaddata.base_url, 0x00, MAXPDSTRING);
			memset(x->oauth.client_key, 0x00, MAXPDSTRING);
			memset(x->oauth.client_secret, 0x00, MAXPDSTRING);
			memset(x->oauth.token_key, 0x00, MAXPDSTRING);
			memset(x->oauth.token_secret, 0x00, MAXPDSTRING);
			break;
		case 3:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->threaddata.base_url, MAXPDSTRING);
			}
			if (argvec[1].a_type != A_SYMBOL) {
				error("Client key cannot be set.");
			} else {
				atom_string(argvec + 1, x->oauth.client_key, MAXPDSTRING);
			}
			if (argvec[2].a_type != A_SYMBOL) {
				error("Client secret cannot be set.");
			} else {
				atom_string(argvec + 2, x->oauth.client_secret, MAXPDSTRING);
			}
			memset(x->oauth.token_key, 0x00, MAXPDSTRING);
			memset(x->oauth.token_secret, 0x00, MAXPDSTRING);
			break;
		case 5:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->threaddata.base_url, MAXPDSTRING);
			}
			if (argvec[1].a_type != A_SYMBOL) {
				error("Client key cannot be set.");
			} else {
				atom_string(argvec + 1, x->oauth.client_key, MAXPDSTRING);
			}
			if (argvec[2].a_type != A_SYMBOL) {
				error("Client secret cannot be set.");
			} else {
				atom_string(argvec + 2, x->oauth.client_secret, MAXPDSTRING);
			}
			if (argvec[3].a_type != A_SYMBOL) {
				error("Token key cannot be set.");
			} else {
				atom_string(argvec + 3, x->oauth.token_key, MAXPDSTRING);
			}
			if (argvec[4].a_type != A_SYMBOL) {
				error("Token secret cannot be set.");
			} else {
				atom_string(argvec + 4, x->oauth.token_secret, MAXPDSTRING);
			}
			break;
		default:
			error("Wrong number of parameters.");
			break;
	}
}

void oauth_setup(void) {
	oauth_class = class_new(gensym("oauth"), (t_newmethod)oauth_new,
			(t_method)oauth_free, sizeof(t_oauth), 0, A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_url, gensym("url"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("POST"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_method, gensym("method"), A_GIMME, 0);
}

void oauth_command(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *request_type;
	char path[MAXPDSTRING];
	char req_path[MAXPDSTRING];
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;
	t_atom auth_status_data[2];
	char *postargs = NULL;
	char *req_url = NULL;

	if(x->threaddata.is_data_locked) {
		post("oauth object is performing request and locked");
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
				if (x->threaddata.base_url != NULL) {
					strcpy(req_path, x->threaddata.base_url);
				}
				strcat(req_path, path);
				if (argcount > 1) {
					atom_string(argvec + 1, parameters, MAXPDSTRING);
					if (strlen(parameters)) {
						cleaned_parameters = remove_backslashes(parameters, &memsize);
						if (strchr(req_path, '?')) {
							strcat(req_path, "&");
						} else {
							strcat(req_path, "?");
						}
						strcat(req_path, cleaned_parameters);
						freebytes(cleaned_parameters, memsize);
					}
				}
				strcpy(x->threaddata.request_type, request_type);
				if ((strcmp(x->threaddata.request_type, "GET") && 
							strcmp(x->threaddata.request_type, "POST"))) {
					SETSYMBOL(&auth_status_data[0], gensym("oauth"));
					SETSYMBOL(&auth_status_data[1], gensym("Request Method not supported"));
					error("Request method %s not supported.", x->threaddata.request_type);
					outlet_list(x->threaddata.status_info_outlet, &s_list, 2, &auth_status_data[0]);
					x->threaddata.is_data_locked = 0;
				} else {
					if (strcmp(x->threaddata.request_type, "POST") == 0) {
						req_url= oauth_sign_url2(req_path, &postargs, x->oauth.method, NULL, 
								x->oauth.client_key, x->oauth.client_secret, 
								x->oauth.token_key, x->oauth.token_secret);
						strcpy(x->threaddata.complete_url, req_url);
						strcpy(x->threaddata.parameters, postargs);
					} else {
						req_url = oauth_sign_url2(req_path, NULL, x->oauth.method, NULL, 
								x->oauth.client_key, x->oauth.client_secret, 
								x->oauth.token_key, x->oauth.token_secret);
						strcpy(x->threaddata.complete_url, req_url);
						memset(x->threaddata.parameters, 0x00, MAXPDSTRING);
					}
					if (postargs) {
						free(postargs);
					}
					if (req_url) {
						free(req_url);
					}
					thread_execute((struct _rest_common *)x, execute_request);
				}
				break;
		}
	}
}

void oauth_method(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char method_name[11];

	(void) selector;

	if (argcount > 0) {
		if (argvec[0].a_type == A_SYMBOL) {
			atom_string(argvec, method_name, 11);
			if (strcmp(method_name, "HMAC") == 0) {
				x->oauth.method = OA_HMAC;
				if (x->oauth.rsa_key) {
					free(x->oauth.rsa_key);
				}
				if (argcount > 1)  {
					post("Additional data is ignored");
				}
			} else if (strcmp(method_name, "PLAINTEXT") == 0) {
				x->oauth.method = OA_PLAINTEXT;
				if (x->oauth.rsa_key) {
					free(x->oauth.rsa_key);
				}
				post("Warning: You are using plaintext now");
				if (argcount > 1)  {
					post("Additional data is ignored");
				}
			} else if (strcmp(method_name, "RSA") == 0) {
				if (argcount > 1) {
					x->oauth.method = OA_RSA;
					/* TODO: Read RSA key */
				} else {
					error("RSA needs the RSA private key as additional data");
				}
			} else {
				error("Only HMAC, RSA, and PLAINTEXT allowed");
			}

		} else {
			error("'method' only takes a symbol argument. See help for more");
		}
	} else 
		error("'method' needs at least one argument. See help for more");
}

void oauth_url(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec) {

	(void) selector;

	if(x->threaddata.is_data_locked) {
		post("oauth object is performing request and locked");
	} else {
		set_url_parameters(x, argcount, argvec); 
	}
}

void *oauth_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_oauth *x = (t_oauth *)pd_new(oauth_class);

	(void) selector;

	set_url_parameters(x, 0, argvec); 
	set_url_parameters(x, argcount, argvec); 
	x->oauth.method = OA_HMAC;
	x->oauth.rsa_key = NULL;

	outlet_new(&x->threaddata.x_ob, NULL);
	x->threaddata.status_info_outlet = outlet_new(&x->threaddata.x_ob, NULL);
	x->threaddata.is_data_locked = 0;

	return (void *)x;
}

void oauth_free(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	if (x->oauth.rsa_key) {
		free(x->oauth.rsa_key);
	}

}
