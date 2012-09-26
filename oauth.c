/*
 * [oauth] communicates with OAUTH webservices via GET and POST.
 * */

#include "purest_json.h"

static t_class *oauth_class;

static void *execute_oauth_request(void *thread_args) {
	t_oauth *x = (t_oauth *)thread_args; 
	char *req_url = NULL;
	char *postargs = NULL;
	char *reply = NULL;
	t_atom http_status_data[3];
		
	if (x->parameters) {
		if (strchr(x->complete_url, '?')) {
			strcat(x->complete_url, "&");
		} else {
			strcat(x->complete_url, "?");
		}
		strcat(x->complete_url, x->parameters);
	}

	if (strcmp(x->request_type, "POST") == 0) {
		req_url = oauth_sign_url2(x->complete_url, &postargs, OA_HMAC, NULL, 
				x->oauth.client_key, x->oauth.client_secret, 
				x->oauth.token_key, x->oauth.token_secret);
		reply = oauth_http_post(req_url, postargs);
	} else {
		req_url = oauth_sign_url2(x->complete_url, NULL, OA_HMAC, NULL, 
				x->oauth.client_key, x->oauth.client_secret, 
				x->oauth.token_key, x->oauth.token_secret);
		reply = oauth_http_get(req_url, NULL);
	}
	SETSYMBOL(&http_status_data[0], gensym("oauth"));

	if (!reply) {
		SETSYMBOL(&http_status_data[1], gensym("no reply"));
		error("Request did not return value");
	} else {
		SETSYMBOL(&http_status_data[1], gensym("bang"));
		outlet_symbol(x->x_ob.ob_outlet, gensym(reply));
	}
	outlet_list(x->status_info_outlet, &s_list, 2, &http_status_data[0]);
	if (postargs) {
		free(postargs);
	}
	if (req_url) {
		free(req_url);
	}
	if (reply) {
		free(reply);
	}
	x->is_data_locked = 0;
	return NULL;
}

static void thread_execute(t_oauth *x, void *(*func) (void *)) {
	int rc;
	pthread_t thread;
	pthread_attr_t thread_attributes;

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&thread, &thread_attributes, func, (void *)x);
	pthread_attr_destroy(&thread_attributes);
	if (rc) {
		error("Could not create thread with code %d", rc);
		x->is_data_locked = 0;
	}
}

static void set_url_parameters(t_oauth *x, int argcount, t_atom *argvec) {
	switch (argcount) {
		case 0:
			memset(x->base_url, 0x00, MAXPDSTRING);
			memset(x->oauth.client_key, 0x00, MAXPDSTRING);
			memset(x->oauth.client_secret, 0x00, MAXPDSTRING);
			memset(x->oauth.token_key, 0x00, MAXPDSTRING);
			memset(x->oauth.token_secret, 0x00, MAXPDSTRING);
			break;
		case 3:
			if (argvec[0].a_type != A_SYMBOL) {
				error("Base URL cannot be set.");
			} else {
				atom_string(argvec, x->base_url, MAXPDSTRING);
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
				atom_string(argvec, x->base_url, MAXPDSTRING);
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
			0, sizeof(t_oauth), 0, A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_url, gensym("url"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("POST"), A_GIMME, 0);
}

void oauth_command(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *request_type;
	char path[MAXPDSTRING];
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;
	t_atom auth_status_data[2];

	if(x->is_data_locked) {
		post("oauth object is performing request and locked");
	} else {
		memset(x->request_type, 0x00, 5);
		memset(x->parameters, 0x00, MAXPDSTRING);
		memset(x->complete_url, 0x00, MAXPDSTRING);
		switch (argcount) {
			case 0:
				break;
			default:
				request_type = selector->s_name;
				atom_string(argvec, path, MAXPDSTRING);
				x->is_data_locked = 1;
				if (argcount > 1) {
					atom_string(argvec + 1, parameters, MAXPDSTRING);
					if (strlen(parameters)) {
						cleaned_parameters = remove_backslashes(parameters, memsize);
						strcpy(x->parameters, cleaned_parameters);
						freebytes(cleaned_parameters, memsize);
					}
				}
				if (x->base_url != NULL) {
					strcpy(x->complete_url, x->base_url);
				}
				strcat(x->complete_url, path);
				strcpy(x->request_type, request_type);
				if ((strcmp(x->request_type, "GET") && 
							strcmp(x->request_type, "POST"))) {
					SETSYMBOL(&auth_status_data[0], gensym("oauth"));
					SETSYMBOL(&auth_status_data[1], gensym("Request Method not supported"));
					error("Request method %s not supported.", x->request_type);
					outlet_list(x->status_info_outlet, &s_list, 2, &auth_status_data[0]);
					x->is_data_locked = 0;
				} else {
					thread_execute(x, execute_oauth_request);
				}
				break;
		}
	}
}

void oauth_url(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec) {

	(void) selector;

	if(x->is_data_locked) {
		post("oauth object is performing request and locked");
	} else {
		set_url_parameters(x, argcount, argvec); 
	}
}

void *oauth_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_oauth *x = (t_oauth *)pd_new(oauth_class);

	(void) selector;

	set_url_parameters(x, argcount, argvec); 

	outlet_new(&x->x_ob, NULL);
	x->status_info_outlet = outlet_new(&x->x_ob, NULL);
	x->is_data_locked = 0;

	return (void *)x;
}
