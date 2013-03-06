/*
 * [oauth] communicates with OAUTH webservices via GET and POST.
 * */

#include "oauth.h"

#include "curl_thread_wrapper.c"
#include "shared_functions.c"

static t_class *oauth_class;

struct _oauth {
	struct _rest_common common;
	/* authentication */
	struct {
		size_t client_key_len;
		char *client_key;
		size_t client_secret_len;
		char *client_secret;
		size_t token_key_len;
		char *token_key;
		size_t token_secret_len;
		char *token_secret;
		OAuthMethod method;
		size_t rsa_key_len;
		char *rsa_key;
	} oauth;
};

static void oauth_free_inner(t_oauth *x, short free_rsa) {
	rest_common_free((struct _rest_common *)x);
	if (free_rsa == 1) {
		free_string(x->oauth.rsa_key, &x->oauth.rsa_key_len);
	}
}

static void set_url_parameters(t_oauth *x, int argc, t_atom *argv) {
	char temp[MAXPDSTRING];

	oauth_free_inner(x, 0);
	switch (argc) {
		case 0:
			break;
		case 3:
			if (argv[0].a_type != A_SYMBOL) {
				MYERROR("Base URL cannot be set.");
			} else {
				atom_string(argv, temp, MAXPDSTRING);
				x->common.base_url = get_string(&x->common.base_url_len, strlen(temp));
				strcpy(x->common.base_url, temp);
			}
			if (argv[1].a_type != A_SYMBOL) {
				MYERROR("Client key cannot be set.");
			} else {
				atom_string(argv + 1, temp, MAXPDSTRING);
				x->oauth.client_key = get_string(&x->oauth.client_key_len, strlen(temp));
				strcpy(x->oauth.client_key, temp);
			}
			if (argv[2].a_type != A_SYMBOL) {
				MYERROR("Client secret cannot be set.");
			} else {
				atom_string(argv + 2, temp, MAXPDSTRING);
				x->oauth.client_secret = get_string(&x->oauth.client_secret_len, strlen(temp));
				strcpy(x->oauth.client_secret, temp);
			}
			break;
		case 5:
			if (argv[0].a_type != A_SYMBOL) {
				MYERROR("Base URL cannot be set.");
			} else {
				atom_string(argv, temp, MAXPDSTRING);
				x->common.base_url = get_string(&x->common.base_url_len, strlen(temp));
				strcpy(x->common.base_url, temp);
			}
			if (argv[1].a_type != A_SYMBOL) {
				MYERROR("Client key cannot be set.");
			} else {
				atom_string(argv + 1, temp, MAXPDSTRING);
				x->oauth.client_key = get_string(&x->oauth.client_key_len, strlen(temp));
				strcpy(x->oauth.client_key, temp);
			}
			if (argv[2].a_type != A_SYMBOL) {
				MYERROR("Client secret cannot be set.");
			} else {
				atom_string(argv + 2, temp, MAXPDSTRING);
				x->oauth.client_secret = get_string(&x->oauth.client_secret_len, strlen(temp));
				strcpy(x->oauth.client_secret, temp);
			}
			if (argv[3].a_type != A_SYMBOL) {
				MYERROR("Client key cannot be set.");
			} else {
				atom_string(argv + 3, temp, MAXPDSTRING);
				x->oauth.token_key = get_string(&x->oauth.token_key_len, strlen(temp));
				strcpy(x->oauth.token_key, temp);
			}
			if (argv[4].a_type != A_SYMBOL) {
				MYERROR("Client secret cannot be set.");
			} else {
				atom_string(argv + 4, temp, MAXPDSTRING);
				x->oauth.token_secret = get_string(&x->oauth.token_secret_len, strlen(temp));
				strcpy(x->oauth.token_secret, temp);
			}
			break;
		default:
			MYERROR("Wrong number of parameters.");
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
	class_addmethod(oauth_class, (t_method)oauth_timeout, gensym("timeout"), A_GIMME, 0);
}

void oauth_command(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {
	char *req_type;
	char path[MAXPDSTRING];
	size_t req_path_len;
	char *req_path;
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;
	t_atom auth_status_data[2];
	char *postargs = NULL;
	char *req_url = NULL;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
	} else {
		memset(x->common.req_type, 0x00, REQUEST_TYPE_LEN);
		switch (argc) {
			case 0:
				break;
			default:
				x->common.locked = 1;
				req_type = sel->s_name;
				strcpy(x->common.req_type, req_type);
				if ((strcmp(x->common.req_type, "GET") && 
							strcmp(x->common.req_type, "POST"))) {
					SETSYMBOL(&auth_status_data[0], gensym("oauth"));
					SETSYMBOL(&auth_status_data[1], gensym("Request Method not supported"));
					MYERROR("Request method %s not supported.", x->common.req_type);
					outlet_list(x->common.stat_out, &s_list, 2, &auth_status_data[0]);
					x->common.locked = 0;
				} else {
					atom_string(argv, path, MAXPDSTRING);
					if (argc > 1) {
						atom_string(argv + 1, parameters, MAXPDSTRING);
						if (strlen(parameters)) {
							cleaned_parameters = remove_backslashes(parameters, &memsize);
						}
					}
					req_path = get_string(&req_path_len, 
							x->common.base_url_len + strlen(path) + memsize + 1);
					if (x->common.base_url != NULL) {
						strcpy(req_path, x->common.base_url);
					}
					strcat(req_path, path);
					if (memsize) {
						if (strchr(req_path, '?')) {
							strcat(req_path, "&");
						} else {
							strcat(req_path, "?");
						}
						strcat(req_path, cleaned_parameters);
						freebytes(cleaned_parameters, memsize);
					}
					if (strcmp(x->common.req_type, "POST") == 0) {
						if (x->oauth.method == OA_RSA) {
							req_url= oauth_sign_url2(req_path, &postargs, x->oauth.method, x->common.req_type, 
									x->oauth.client_key, x->oauth.rsa_key, 
									x->oauth.token_key, NULL);
						} else {
							req_url= oauth_sign_url2(req_path, &postargs, x->oauth.method, x->common.req_type, 
									x->oauth.client_key, x->oauth.client_secret, 
									x->oauth.token_key, x->oauth.token_secret);
						}
					} else {
						if (x->oauth.method == OA_RSA) {
							req_url= oauth_sign_url2(req_path, NULL, x->oauth.method, x->common.req_type, 
									x->oauth.client_key, x->oauth.rsa_key, 
									x->oauth.token_key, NULL);
							post("rsa_key: %s", x->oauth.rsa_key);
							post("url: %s", req_url);
						} else {
							req_url= oauth_sign_url2(req_path, NULL, x->oauth.method, x->common.req_type, 
									x->oauth.client_key, x->oauth.client_secret, 
									x->oauth.token_key, x->oauth.token_secret);
						}
					}
					x->common.complete_url = get_string(&x->common.complete_url_len, strlen(req_url));
					strcpy(x->common.complete_url, req_url);
					if (strcmp(x->common.req_type, "POST") == 0) {
						x->common.parameters = get_string(&x->common.parameters_len, strlen(postargs));
						strcpy(x->common.parameters, postargs);
					} else {
						x->common.parameters = get_string(&x->common.parameters_len, 0);
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

void oauth_method(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {
	char method_name[11];
	char temp[MAXPDSTRING];
	size_t rsa_key_len = 1;
	int i;
	short use_newline = 0;

	(void) sel;

	free_string(x->oauth.rsa_key, &x->oauth.rsa_key_len);

	if (argc > 0) {
		if (argv[0].a_type == A_SYMBOL) {
			atom_string(argv, method_name, 11);
			if (strcmp(method_name, "HMAC") == 0) {
				x->oauth.method = OA_HMAC;
				if (argc > 1)  {
					post("Additional data is ignored");
				}
			} else if (strcmp(method_name, "PLAINTEXT") == 0) {
				x->oauth.method = OA_PLAINTEXT;
				post("Warning: You are using plaintext now");
				if (argc > 1)  {
					post("Additional data is ignored");
				}
			} else if (strcmp(method_name, "RSA") == 0) {
				MYERROR("RSA-SHA1 is not supported by liboauth");
				if (argc > 1) {
					x->oauth.method = OA_RSA;
					for (i = 1; i < argc; i++) {
						atom_string(argv + i, temp, MAXPDSTRING);
						rsa_key_len +=strlen(temp) + 1;
					}
					x->oauth.rsa_key = get_string(&x->oauth.rsa_key_len, rsa_key_len);
					for (i = 1; i < argc; i++) {
						atom_string(argv + i, temp, MAXPDSTRING);
						if (strncmp(temp, "-----", 5) == 0 && strlen(x->oauth.rsa_key) > 1)  {
							memset(x->oauth.rsa_key + strlen(x->oauth.rsa_key) - 1, 0x00, 1);
							strcat(x->oauth.rsa_key, "\n");
							use_newline = 0;
						}
						if (strlen(temp) >= 5 && strncmp(temp + strlen(temp) - 5, "-----", 5) == 0) {
							use_newline = 1;
						}
						strcat(x->oauth.rsa_key, temp);
						if (i < argc -1) {
							if (use_newline == 1)  {
								strcat(x->oauth.rsa_key, "\n");
							} else {
								strcat(x->oauth.rsa_key, " ");
							}
						}
					}
				} else {
					MYERROR("RSA needs the RSA private key as additional data");
				}
			} else {
				MYERROR("Only HMAC, RSA, and PLAINTEXT allowed");
			}

		} else {
			MYERROR("'method' only takes a symbol argument. See help for more");
		}
	} else  {
		MYERROR("'method' needs at least one argument. See help for more");
	}
}

void oauth_url(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
	} else {
		set_url_parameters(x, argc, argv); 
	}
}

void oauth_timeout(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
	} else if (argc > 2){
		MYERROR("timeout must have 0 or 1 parameter");
	} else if (argc == 0) {
		set_timeout((struct _rest_common *)x, 0);
	} else {
		set_timeout((struct _rest_common *)x, atom_getint(argv));
	}
}

void *oauth_new(t_symbol *sel, int argc, t_atom *argv) {
	t_oauth *x = (t_oauth *)pd_new(oauth_class);

	(void) sel;

	init_common((struct _rest_common *)x);
	set_timeout((struct _rest_common *)x, 0);

	set_url_parameters(x, 0, argv); 
	set_url_parameters(x, argc, argv); 
	x->oauth.method = OA_HMAC;
	x->oauth.rsa_key_len = 0;

	outlet_new(&x->common.x_ob, NULL);
	x->common.stat_out = outlet_new(&x->common.x_ob, NULL);
	x->common.locked = 0;

	return (void *)x;
}

void oauth_free(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	oauth_free_inner(x, 1);
}
