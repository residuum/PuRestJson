/*
 * [oauth] communicates with OAUTH webservices via GET and POST.
 * */

#include "oauth.h"

#include "string.c"
#include "strlist.c"
#include "ctw.c"

static t_class *oauth_class;

struct _oauth {
	struct _ctw common;
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
	ctw_free((struct _ctw *)x);
	if (free_rsa == 1) {
		string_free(x->oauth.rsa_key, &x->oauth.rsa_key_len);
	}
}

static void oauth_set_url_params(t_oauth *x, int argc, t_atom *argv) {
	oauth_free_inner(x, 0);

	switch (argc) {
		case 0:
			break;
		case 5:
			x->oauth.token_key = ctw_set_param((void *)x, argv + 3, &x->oauth.token_key_len, "Token key cannot be set.");
			x->oauth.token_secret = ctw_set_param((void *)x, argv + 4, &x->oauth.token_secret_len, "Token secret cannot be set.");
			/* fall through deliberately */
		case 3:
			x->common.base_url = ctw_set_param((void *)x, argv, &x->common.base_url_len, "Base URL cannot be set.");
			x->oauth.client_key = ctw_set_param((void *)x, argv + 1, &x->oauth.client_key_len, "Client key cannot be set.");
			x->oauth.client_secret = ctw_set_param((void *)x, argv + 2, &x->oauth.client_secret_len, "Client secret cannot be set.");
			break;
		default:
			pd_error(x, "Wrong number of parameters.");
			break;
	}
}

static void oauth_set_rsa_key(t_oauth *x, int argc, t_atom *argv) {
	char temp[MAXPDSTRING];
	int i;
	size_t rsa_key_len = 1;
	short use_newline = 0;

	for (i = 1; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		rsa_key_len +=strlen(temp) + 1;
	}
	x->oauth.rsa_key = string_create(&x->oauth.rsa_key_len, rsa_key_len);
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
}

void oauth_setup(void) {
	oauth_class = class_new(gensym("oauth"), (t_newmethod)oauth_new,
			(t_method)oauth_free, sizeof(t_oauth), 0, A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_url, gensym("url"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("POST"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_method, gensym("method"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_timeout, gensym("timeout"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_sslcheck, gensym("sslcheck"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_cancel, gensym("cancel"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_header, gensym("header"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_clear_headers, gensym("header_clear"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_write, gensym("write"), A_GIMME, 0);
	class_sethelpsymbol(oauth_class, gensym("rest"));
}

void oauth_command(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {
	char *req_type;
	char path[MAXPDSTRING];
	size_t req_path_len;
	char *req_path;
	char parameters[MAXPDSTRING];
	char *cleaned_parameters;
	size_t memsize = 0;
	char *postargs = NULL;
	char *req_url = NULL;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
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
				strcmp(x->common.req_type, "POST"))) {
		pd_error(x, "Request method %s not supported.", x->common.req_type);
		x->common.locked = 0;
		return;
	}

	atom_string(argv, path, MAXPDSTRING);
	if (argc > 1) {
		atom_string(argv + 1, parameters, MAXPDSTRING);
		if (strlen(parameters)) {
			cleaned_parameters = string_remove_backslashes(parameters, &memsize);
		}
	}
	req_path = string_create(&req_path_len, 
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
		} else {
			req_url= oauth_sign_url2(req_path, NULL, x->oauth.method, x->common.req_type, 
					x->oauth.client_key, x->oauth.client_secret, 
					x->oauth.token_key, x->oauth.token_secret);
		}
	}
	x->common.complete_url = string_create(&x->common.complete_url_len, strlen(req_url));
	strcpy(x->common.complete_url, req_url);
	if (strcmp(x->common.req_type, "POST") == 0) {
		x->common.parameters = string_create(&x->common.parameters_len, strlen(postargs));
		strcpy(x->common.parameters, postargs);
	} else {
		x->common.parameters = string_create(&x->common.parameters_len, 0);
	}
	if (postargs) {
		free(postargs);
	}
	if (req_url) {
		free(req_url);
	}
	ctw_thread_exec((void *)x, ctw_exec);
}

void oauth_method(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {
	char method_name[11];

	(void) sel;

	string_free(x->oauth.rsa_key, &x->oauth.rsa_key_len);

	if (argc == 0) {
		pd_error(x, "'method' needs at least one argument. See help for more");
		return;
	}

	if (argv[0].a_type != A_SYMBOL) {
		pd_error(x, "'method' only takes a symbol argument. See help for more");
		return;
	}
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
		if (LIBOAUTH_VERSION_MAJOR < 1
				|| (LIBOAUTH_VERSION_MAJOR == 1 
					&& LIBOAUTH_VERSION_MINOR == 0 
					&& LIBOAUTH_VERSION_MICRO == 0)) {
			pd_error(x, "RSA-SHA1 is not supported by liboauth version < 1.0.1");
			return;
		}
		if (argc > 1) {
			x->oauth.method = OA_RSA;
			oauth_set_rsa_key(x, argc, argv);
		} else {
			pd_error(x, "RSA needs the RSA private key as additional data");
		}
	} else {
		pd_error(x, "Only HMAC, RSA, and PLAINTEXT allowed");
	}
}

void oauth_url(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
	} else {
		oauth_set_url_params(x, argc, argv); 
	}
}

void oauth_timeout(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
	} else if (argc > 2){
		pd_error(x, "timeout must have 0 or 1 parameter");
	} else if (argc == 0) {
		ctw_set_timeout((struct _ctw *)x, 0);
	} else {
		ctw_set_timeout((struct _ctw *)x, atom_getint(argv));
	}
}

void oauth_sslcheck(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	if(x->common.locked) {
		post("oauth object is performing request and locked");
	} else if (argc != 1){
		pd_error(x, "sslcheck must have 1 parameter");
	} else {
		ctw_set_sslcheck((struct _ctw *)x, atom_getint(argv));
	}
}

void oauth_cancel(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_cancel((struct _ctw *)x);
}

void oauth_header(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	ctw_add_header((void *)x, argc, argv);
}

void oauth_clear_headers(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_clear_headers((struct _ctw *)x);
}

void oauth_write(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {

	(void) sel;

	ctw_set_file((void *)x, argc, argv);
}

void *oauth_new(t_symbol *sel, int argc, t_atom *argv) {
	t_oauth *x = (t_oauth *)pd_new(oauth_class);

	(void) sel;

	ctw_init((struct _ctw *)x);
	ctw_set_timeout((struct _ctw *)x, 0);

	oauth_set_url_params(x, 0, argv); 
	oauth_set_url_params(x, argc, argv); 
	x->oauth.method = OA_HMAC;
	x->oauth.rsa_key_len = 0;

	outlet_new(&x->common.x_ob, NULL);
	x->common.status_out = outlet_new(&x->common.x_ob, NULL);
	x->common.locked = 0;
#ifdef NEEDS_CERT_PATH
	ctw_set_cert_path((struct _ctw *)x, oauth_class->c_externdir->s_name);
#endif
	return (void *)x;
}

void oauth_free(t_oauth *x, t_symbol *sel, int argc, t_atom *argv) {
	(void) sel;
	(void) argc;
	(void) argv;

	oauth_free_inner(x, 1);
}
