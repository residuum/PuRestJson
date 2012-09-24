/*
 * [rest-json] makes RESTful calls to webservices that work with JSON data.
 * */

#include "purest_json.h"

static t_class *rest_class;

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	t_memory_struct *mem = (t_memory_struct *)data;

	mem->memory = (char *) resizebytes(mem->memory, mem->size, mem->size + realsize + sizeof(char));
	if (mem->memory == NULL) {
		/* out of memory! */ 
		error("not enough memory");
	}
	memcpy(&mem->memory[mem->size], ptr, realsize);
	if (mem->size + realsize - mem->size != realsize) {
		error("Integer overflow or similar. Bad Things can happen.");
	}
	mem->size += realsize;
	mem->memory[mem->size] = '\0';

	return realsize;
}

static size_t read_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	t_memory_struct *mem = (t_memory_struct *)data;
	size_t to_copy = (mem->size < realsize) ? mem->size : realsize;

	memcpy(ptr, mem->memory, to_copy);
	mem->size -= to_copy;
	mem->memory += to_copy;
	return to_copy;
}

static void *execute_oauth_request(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	char *req_url = NULL;
	char *postargs = NULL;
	char *reply = NULL;
	t_atom http_status_data[3];

	if (strcmp(x->request_type, "POST") == 0) {
		if (x->parameters) {
			if (strchr(x->complete_url, '?')) {
				strcat(x->complete_url, "&");
			} else {
				strcat(x->complete_url, "?");
			}
			strcat(x->complete_url, x->parameters);
		}
		req_url = oauth_sign_url2(x->complete_url, &postargs, OA_HMAC, NULL, 
				x->auth.oauth.client_key, x->auth.oauth.client_secret, 
				x->auth.oauth.token_key, x->auth.oauth.token_secret);
		reply = oauth_http_post(req_url, postargs);
	} else {
		req_url = oauth_sign_url2(x->complete_url, NULL, OA_HMAC, NULL, 
				x->auth.oauth.client_key, x->auth.oauth.client_secret, 
				x->auth.oauth.token_key, x->auth.oauth.token_secret);
		reply = oauth_http_get(req_url, NULL);
	}
	SETSYMBOL(&http_status_data[0], gensym("oauth"));

	if (!reply) {
		SETSYMBOL(&http_status_data[1], gensym("no reply"));
		error("Request did not return value");
	} else {
		SETSYMBOL(&http_status_data[1], gensym("bang"));
		output_json_string(reply, x->x_ob.ob_outlet, x->done_outlet);
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

static void *execute_rest_request(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	t_memory_struct in_memory;
	t_memory_struct out_memory;
	long http_code;
	t_atom http_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, x->complete_url);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
		if (strlen(x->auth.cookie.auth_token) != 0) {
			curl_easy_setopt(curl_handle, CURLOPT_COOKIE, x->auth.cookie.auth_token);
		}
		if (strcmp(x->request_type, "PUT") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, TRUE);
			curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_memory_callback);
			/* Prepare data for reading */
			in_memory.memory = getbytes(strlen(x->parameters) + 1);
			if (in_memory.memory == NULL) {
				error("not enough memory");
			}
			in_memory.size = strlen(x->parameters);
			memcpy(in_memory.memory, x->parameters, strlen(x->parameters));
			curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&in_memory);
		} else if (strcmp(x->request_type, "POST") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
		} else if (strcmp(x->request_type, "DELETE") == 0) {
			curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}
		out_memory.memory = getbytes(1);
		out_memory.size = 0;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&out_memory);
		result = curl_easy_perform(curl_handle);

		/* output status */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
		SETSYMBOL(&http_status_data[0], gensym(x->request_type));
		if (http_code == 200) {
			SETSYMBOL(&http_status_data[1], gensym("bang"));
			outlet_list(x->status_info_outlet, &s_list, 2, &http_status_data[0]);
			if (result == CURLE_OK) {
				output_json_string(out_memory.memory, x->x_ob.ob_outlet, x->done_outlet);
				/* Free memory */
				if (out_memory.memory) {
					freebytes(out_memory.memory, (out_memory.size + 1) * sizeof(char));
				}
				free((void *)result);
			} else {
				SETFLOAT(&http_status_data[1], (float)http_code);
				SETFLOAT(&http_status_data[2], (float)result);
				outlet_list(x->status_info_outlet, &s_list, 3, &http_status_data[0]);
				error("Error while performing request: %s", curl_easy_strerror(result));
			}
		} else {
			SETFLOAT(&http_status_data[1], (float)http_code);
			SETFLOAT(&http_status_data[2], (float)result);
			error("Error while performing request: %s", curl_easy_strerror(result));
			outlet_list(x->status_info_outlet, &s_list, 3, &http_status_data[0]);
		}
		curl_easy_cleanup(curl_handle);
	} else {
		error("Cannot init curl.");
	}
	x->is_data_locked = 0;
	return NULL;
}

static void *get_cookie_auth_token(void *thread_args) {
	t_rest *x = (t_rest *)thread_args; 
	CURL *curl_handle;
	CURLcode result;
	t_memory_struct out_content;
	t_memory_struct out_header;
	char *post_data;
	size_t post_data_length;
	char *header_line;
	char *cookie_params;
	long http_code;
	t_atom auth_status_data[3];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		memset(x->complete_url, 0x00, MAXPDSTRING);
		post_data_length = strlen(x->auth.cookie.username) + strlen(x->auth.cookie.password) + 17;/*name=&password=*/
		post_data = (char *)getbytes(sizeof(char) * post_data_length);
		if (post_data == NULL) {
			error("not enough memory");
		} else {
			strcpy(post_data, "name=");
			strcat(post_data, x->auth.cookie.username);
			strcat(post_data, "&password=");
			strcat(post_data, x->auth.cookie.password);
		}
		strcpy(x->complete_url, x->base_url);
		strcat(x->complete_url, x->auth.cookie.login_path);
		curl_easy_setopt(curl_handle, CURLOPT_URL, x->complete_url);
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

		/* output the status code at third outlet */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
		SETSYMBOL(&auth_status_data[0], gensym("cookie"));
		if (http_code == 200 && result == CURLE_OK) {
			SETSYMBOL(&auth_status_data[1], gensym("bang"));
			outlet_list(x->status_info_outlet, &s_list, 2, &auth_status_data[0]);
		} else {
			SETFLOAT(&auth_status_data[1], (float)http_code);
			SETFLOAT(&auth_status_data[2], (float)result);
			outlet_list(x->status_info_outlet, &s_list, 3, &auth_status_data[0]);
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
								strcpy(x->auth.cookie.auth_token, cookie_params);
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
	x->is_data_locked = 0;
	return NULL;
}

static void thread_execute(t_rest *x, void *(*func) (void *)) {
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

static void set_url_parameters(t_rest *x, int argcount, t_atom *argvec) {
	switch (x->auth_type) {
		case COOKIE:
			switch (argcount) {
				case 0:
					memset(x->base_url, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.login_path, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.username, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.password, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.auth_token, 0x00, MAXPDSTRING);
					break;
				case 1:
					if (argvec[0].a_type != A_SYMBOL) {
						error("Base URL cannot be set.");
					} else {
						atom_string(argvec, x->base_url, MAXPDSTRING);
					}
					memset(x->auth.cookie.login_path, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.username, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.password, 0x00, MAXPDSTRING);
					memset(x->auth.cookie.auth_token, 0x00, MAXPDSTRING);
					break;
				case 4:
					x->is_data_locked = 1;
					if (argvec[0].a_type != A_SYMBOL) {
						error("Base URL cannot be set.");
					} else {
						atom_string(argvec, x->base_url, MAXPDSTRING);
					}
					if (argvec[1].a_type != A_SYMBOL) {
						error("Username cannot be set.");
					} else {
						atom_string(argvec + 1, x->auth.cookie.login_path, MAXPDSTRING);
					}
					if (argvec[2].a_type != A_SYMBOL) {
						error("Username cannot be set.");
					} else {
						atom_string(argvec + 2, x->auth.cookie.username, MAXPDSTRING);
					}
					if (argvec[3].a_type != A_SYMBOL) {
						error("Password cannot be set.");
					} else {
						atom_string(argvec + 3, x->auth.cookie.password, MAXPDSTRING);
					}
					thread_execute(x, get_cookie_auth_token);
					break;
				default:
					error("Wrong number of parameters.");
					break;
			}
			break;
		case OAUTH:
			switch (argcount) {
				case 5:
					if (argvec[0].a_type != A_SYMBOL) {
						error("Base URL cannot be set.");
					} else {
						atom_string(argvec, x->base_url, MAXPDSTRING);
					}
					if (argvec[1].a_type != A_SYMBOL) {
						error("Client key cannot be set.");
					} else {
						atom_string(argvec + 1, x->auth.oauth.client_key, MAXPDSTRING);
					}
					if (argvec[2].a_type != A_SYMBOL) {
						error("Client secret cannot be set.");
					} else {
						atom_string(argvec + 2, x->auth.oauth.client_secret, MAXPDSTRING);
					}
					if (argvec[3].a_type != A_SYMBOL) {
						error("Token key cannot be set.");
					} else {
						atom_string(argvec + 3, x->auth.oauth.token_key, MAXPDSTRING);
					}
					if (argvec[4].a_type != A_SYMBOL) {
						error("Token secret cannot be set.");
					} else {
						atom_string(argvec + 4, x->auth.oauth.token_secret, MAXPDSTRING);
					}
					break;
				default:
					error("Wrong number of parameters.");
					break;
			}
			break;
	}
}

void setup_rest0x2djson(void) {
	rest_class = class_new(gensym("rest-json"), (t_newmethod)rest_new,
			0, sizeof(t_rest), 0, A_GIMME, 0);
	class_addmethod(rest_class, (t_method)rest_oauth, gensym("oauth"), A_GIMME, 0);
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

	if(x->is_data_locked) {
		post("rest-json object is performing request and locked");
	} else {
		memset(x->request_type, 0x00, 7);
		memset(x->parameters, 0x00, MAXPDSTRING);
		memset(x->complete_url, 0x00, MAXPDSTRING);
		switch (argcount) {
			case 0:
				break;
			default:
				request_type = selector->s_name;
				atom_string(argvec, path, MAXPDSTRING);
				if (argcount > 1) {
					atom_string(argvec + 1, parameters, MAXPDSTRING);
				}
				x->is_data_locked = 1;
				if (x->base_url != NULL) {
					strcpy(x->complete_url, x->base_url);
				}
				strcat(x->complete_url, path);
				strcpy(x->request_type, request_type);
				if (parameters != NULL) {
					cleaned_parameters = remove_backslashes(parameters, memsize);
					strcpy(x->parameters, cleaned_parameters);
					freebytes(cleaned_parameters, memsize);
				}
				switch(x->auth_type) {
					case COOKIE: 
						if ((strcmp(x->request_type, "GET") && 
								strcmp(x->request_type, "POST") && 
								strcmp(x->request_type, "PUT") &&
								strcmp(x->request_type, "DELETE"))) {
							SETSYMBOL(&auth_status_data[0], gensym("request"));
							SETSYMBOL(&auth_status_data[1], gensym("Request method not supported"));
							error("Request method %s not supported.", x->request_type);
							outlet_list(x->status_info_outlet, &s_list, 2, &auth_status_data[0]);
						} else {
							thread_execute(x, execute_rest_request);
						}
						break;
					case OAUTH:
						if ((strcmp(x->request_type, "GET") && 
								strcmp(x->request_type, "POST"))) {
							SETSYMBOL(&auth_status_data[0], gensym("oauth"));
							SETSYMBOL(&auth_status_data[1], gensym("Request Method not supported"));
							error("Request method %s not supported.", x->request_type);
							outlet_list(x->status_info_outlet, &s_list, 2, &auth_status_data[0]);
						} else {
							thread_execute(x, execute_oauth_request);
						}
						break;
				}
				break;
		}
	}
}

void rest_oauth(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {

	(void) selector;

	if(x->is_data_locked) {
		post("rest-json object is performing request and locked");
	} else {
		x->auth_type = OAUTH;
		set_url_parameters(x, argcount, argvec); 
	}
}

void rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec) {

	(void) selector;

	if(x->is_data_locked) {
		post("rest-json object is performing request and locked");
	} else {
		x->auth_type = COOKIE;
		set_url_parameters(x, argcount, argvec); 
	}
}

void *rest_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_rest *x = (t_rest *)pd_new(rest_class);

	(void) selector;

	x->auth_type = COOKIE;
	set_url_parameters(x, argcount, argvec); 

	outlet_new(&x->x_ob, NULL);
	x->done_outlet = outlet_new(&x->x_ob, &s_bang);
	x->status_info_outlet = outlet_new(&x->x_ob, NULL);
	x->is_data_locked = 0;

	return (void *)x;
}
