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
 * [oauth] communicates with OAUTH webservices via GET and POST.
 * */

#include "oauth.h"

#include "inc/string.c"
#include "inc/strlist.c"
#include "inc/ctw.c"

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

/* frees data */
static void oauth_free_inner(t_oauth *oauth, const short free_rsa);
/* initialises object */
static void oauth_set_init(t_oauth *oauth, const int argc, t_atom *argv);
/* sets RSA key, if OAuth method is set to RSA */
static void oauth_set_rsa_key(t_oauth *oauth, const int argc, t_atom *argv);
/* checks, if method needs input */
static int oauth_needs_input(const char *req_type);

/* begin implementations */
static void oauth_free_inner(t_oauth *const oauth, const short free_rsa) {
	ctw_free((struct _ctw *)oauth);
	if (free_rsa == 1) {
		string_free(oauth->oauth.rsa_key, &oauth->oauth.rsa_key_len);
	}
}

static void oauth_set_init(t_oauth *const oauth, const int argc, t_atom *const argv) {
	oauth_free_inner(oauth, 0);

	switch (argc) {
		case 0:
			break;
		case 5:
			oauth->oauth.token_key = ctw_set_param((struct _ctw *)oauth, argv + 3, 
					&oauth->oauth.token_key_len, "Token key cannot be set.");
			oauth->oauth.token_secret = ctw_set_param((struct _ctw *)oauth, argv + 4, 
					&oauth->oauth.token_secret_len, "Token secret cannot be set.");
			/* fall through deliberately */
		case 3:
			oauth->common.base_url = ctw_set_param((struct _ctw *)oauth, argv, 
					&oauth->common.base_url_len, "Base URL cannot be set.");
			oauth->oauth.client_key = ctw_set_param((struct _ctw *)oauth, argv + 1, 
					&oauth->oauth.client_key_len, "Client key cannot be set.");
			oauth->oauth.client_secret = ctw_set_param((struct _ctw *)oauth, argv + 2, 
					&oauth->oauth.client_secret_len, "Client secret cannot be set.");
			break;
		default:
			pd_error(oauth, "Wrong number of parameters.");
			break;
	}
}

static void oauth_set_rsa_key(t_oauth *const oauth, const int argc, t_atom *const argv) {
	char temp[MAXPDSTRING];
	size_t rsa_key_len = 1;
	short use_newline = 0;

	for (int i = 1; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		rsa_key_len +=strlen(temp) + 1;
	}
	oauth->oauth.rsa_key = string_create(&oauth->oauth.rsa_key_len, rsa_key_len);
	for (int i = 1; i < argc; i++) {
		atom_string(argv + i, temp, MAXPDSTRING);
		if (strncmp(temp, "-----", 5) == 0 && strlen(oauth->oauth.rsa_key) > 1)  {
			memset(oauth->oauth.rsa_key + strlen(oauth->oauth.rsa_key) - 1, 0x00, 1);
			strcat(oauth->oauth.rsa_key, "\n");
			use_newline = 0;
		}
		if (strlen(temp) >= 5 && strncmp(temp + strlen(temp) - 5, "-----", 5) == 0) {
			use_newline = 1;
		}
		strcat(oauth->oauth.rsa_key, temp);
		if (i < argc -1) {
			if (use_newline == 1)  {
				strcat(oauth->oauth.rsa_key, "\n");
			} else {
				strcat(oauth->oauth.rsa_key, " ");
			}
		}
	}
}

static int oauth_needs_input(const char *const req_type) {
	return (strcmp(req_type, "POST")
			&& strcmp(req_type, "PUT")
			&& strcmp(req_type, "PATCH"));
}

void oauth_setup(void) {
	oauth_class = class_new(gensym("oauth"), (t_newmethod)oauth_new,
			(t_method)oauth_free, sizeof(t_oauth), 0, A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_init, gensym("init"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("GET"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("POST"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("HEAD"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("PUT"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("DELETE"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("PATCH"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("OPTIONS"), A_GIMME, 0);
	/*class_addmethod(oauth_class, (t_method)oauth_command, gensym("CONNECT"), A_GIMME, 0);*/
	class_addmethod(oauth_class, (t_method)oauth_command, gensym("TRACE"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_method, gensym("method"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_timeout, gensym("timeout"), A_DEFFLOAT, 0);
	class_addmethod(oauth_class, (t_method)oauth_sslcheck, gensym("sslcheck"), A_DEFFLOAT, 0);
	class_addmethod(oauth_class, (t_method)oauth_cancel, gensym("cancel"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_header, gensym("header"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_clear_headers, gensym("header_clear"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_file, gensym("file"), A_GIMME, 0);
	class_addmethod(oauth_class, (t_method)oauth_mode, gensym("mode"), A_GIMME, 0);
	class_sethelpsymbol(oauth_class, gensym("rest"));
}

void oauth_command(t_oauth *const oauth, const t_symbol *const sel, const int argc, t_atom *argv) {
	char *req_type;
	char path[MAXPDSTRING];
	size_t req_path_len;
	char *req_path;
	char *cleaned_parameters;
	size_t memsize = 0;
	char *postargs = NULL;
	char *req_url = NULL;

	if (oauth->common.locked) {
		post("oauth object is performing request and locked.");
		return;
	}

	memset(oauth->common.req_type, 0x00, REQUEST_TYPE_LEN);
	if (argc == 0) {
		return;
	}

	/* check for initialisation */
	if (oauth->common.base_url == NULL) {
		pd_error(oauth, "oauth object is not initialised.");
		oauth->common.locked = 0;
		return;
	}

	oauth->common.locked = 1;
	req_type = sel->s_name;
	strcpy(oauth->common.req_type, req_type);
	if (ctw_check_request_type(oauth->common.req_type) != 0){
		pd_error(oauth, "Request method %s not supported.", oauth->common.req_type);
		oauth->common.locked = 0;
		return;
	}

	atom_string(argv, path, MAXPDSTRING);
	if (argc > 1) {
		char parameters[MAXPDSTRING];
		atom_string(argv + 1, parameters, MAXPDSTRING);
		if (strlen(parameters)) {
			cleaned_parameters = string_remove_backslashes(parameters, &memsize);
		}
	}
	req_path = string_create(&req_path_len, 
			oauth->common.base_url_len + strlen(path) + memsize + 1);
	if (oauth->common.base_url != NULL) {
		strcpy(req_path, oauth->common.base_url);
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
	if (oauth_needs_input(oauth->common.req_type) == 0) {
		req_url= oauth_sign_url2(req_path, &postargs, oauth->oauth.method, oauth->common.req_type,
				oauth->oauth.client_key,
				oauth->oauth.method == OA_RSA ? oauth->oauth.rsa_key : oauth->oauth.client_secret,
				oauth->oauth.token_key, 
				oauth->oauth.method == OA_RSA ? NULL : oauth->oauth.token_secret);
		oauth->common.parameters = string_create(&oauth->common.parameters_len, strlen(postargs));
		strcpy(oauth->common.parameters, postargs);
	} else {
		req_url= oauth_sign_url2(req_path, NULL, oauth->oauth.method, oauth->common.req_type,
				oauth->oauth.client_key,
				oauth->oauth.method == OA_RSA ? oauth->oauth.rsa_key : oauth->oauth.client_secret,
				oauth->oauth.token_key, 
				oauth->oauth.method == OA_RSA ? NULL : oauth->oauth.token_secret);
		oauth->common.parameters = string_create(&oauth->common.parameters_len, 0);
	}
	oauth->common.complete_url = string_create(&oauth->common.complete_url_len, strlen(req_url));
	strcpy(oauth->common.complete_url, req_url);
	if (postargs) {
		free(postargs);
	}
	free(req_url);
	ctw_thread_exec((struct _ctw *)oauth, ctw_exec);
}

void oauth_method(t_oauth *const oauth, const t_symbol *const sel, const int argc, t_atom *const argv) {
	char method_name[11];

	(void) sel;

	string_free(oauth->oauth.rsa_key, &oauth->oauth.rsa_key_len);

	if (argc == 0) {
		pd_error(oauth, "'method' needs at least one argument. See help for more.");
		return;
	}

	if (argv[0].a_type != A_SYMBOL) {
		pd_error(oauth, "'method' only takes a symbol argument. See help for more.");
		return;
	}
	atom_string(argv, method_name, 11);
	if (strcmp(method_name, "HMAC") == 0) {
		oauth->oauth.method = OA_HMAC;
		if (argc > 1)  {
			post("Additional data is ignored.");
		}
	} else if (strcmp(method_name, "PLAINTEXT") == 0) {
		oauth->oauth.method = OA_PLAINTEXT;
		post("Warning: You are using plaintext now.");
		if (argc > 1)  {
			post("Additional data is ignored.");
		}
	} else if (strcmp(method_name, "RSA") == 0) {
		if (LIBOAUTH_VERSION_MAJOR < 1
				|| (LIBOAUTH_VERSION_MAJOR == 1 
					&& LIBOAUTH_VERSION_MINOR == 0 
					&& LIBOAUTH_VERSION_MICRO == 0)) {
			pd_error(oauth, "RSA-SHA1 is not supported by liboauth version < 1.0.1.");
			return;
		}
		if (argc > 1) {
			oauth->oauth.method = OA_RSA;
			oauth_set_rsa_key(oauth, argc, argv);
		} else {
			pd_error(oauth, "RSA needs the RSA private key as additional data.");
		}
	} else {
		pd_error(oauth, "Only HMAC, RSA, and PLAINTEXT allowed.");
	}
}

void oauth_init(t_oauth *const oauth, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	if (oauth->common.locked) {
		post("oauth object is performing request and locked.");
	} else {
		oauth_set_init(oauth, argc, argv); 
	}
}

void oauth_timeout(t_oauth *const oauth, const t_floatarg f) {
	if (oauth->common.locked) {
		post("oauth object is performing request and locked.");
	} else {
		ctw_set_timeout((struct _ctw *)oauth, (int)f);
	}
}

void oauth_sslcheck(t_oauth *const oauth, const t_floatarg f) {
	if (oauth->common.locked) {
		post("oauth object is performing request and locked.");
	} else {
		ctw_set_sslcheck((struct _ctw *)oauth, (int)f);
	}
}

void oauth_cancel(t_oauth *const oauth, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_cancel((struct _ctw *)oauth);
}

void oauth_header(t_oauth *const oauth, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_add_header((struct _ctw *)oauth, argc, argv);
}

void oauth_clear_headers(t_oauth *const oauth, const t_symbol *const sel, const int argc, 
		const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	ctw_clear_headers((struct _ctw *)oauth);
}

void oauth_file(t_oauth *const oauth, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_set_file((struct _ctw *)oauth, argc, argv);
}

void oauth_mode(t_oauth *const oauth, const t_symbol *const sel, const int argc, t_atom *const argv) {

	(void) sel;

	ctw_set_mode((struct _ctw *)oauth, argc, argv);
}

void *oauth_new(const t_symbol *const sel, const int argc, t_atom *const argv) {
	t_oauth *const oauth = (t_oauth *)pd_new(oauth_class);

	(void) sel;

	ctw_init((struct _ctw *)oauth);
	ctw_set_timeout((struct _ctw *)oauth, 0);

	oauth_set_init(oauth, 0, argv); 
	oauth_set_init(oauth, argc, argv); 
	oauth->oauth.method = OA_HMAC;
	oauth->oauth.rsa_key_len = 0;

	outlet_new(&oauth->common.x_ob, NULL);
	oauth->common.status_out = outlet_new(&oauth->common.x_ob, NULL);
	oauth->common.locked = 0;
#ifdef NEEDS_CERT_PATH
	ctw_set_cert_path((struct _ctw *)oauth, oauth_class->c_externdir->s_name);
#endif
	purest_json_lib_info("oauth");
	return (void *)oauth;
}

void oauth_free(t_oauth *const oauth, const t_symbol *const sel, const int argc, const t_atom *const argv) {

	(void) sel;
	(void) argc;
	(void) argv;

	oauth_free_inner(oauth, 1);
}
