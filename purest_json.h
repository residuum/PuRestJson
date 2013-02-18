#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <pthread.h>
#include <oauth.h>

#define LIBRARY_VERSION "0.9"

#define REQUEST_TYPE_LEN 7

#ifdef _WIN32
	#define APIEXPORT __declspec(dllexport)
	#define APICALL __cdecl
#else 
	#define APIEXPORT
	#define APICALL
#endif

/* [rest] */
struct _rest;
typedef struct _rest t_rest;

APIEXPORT void APICALL rest_setup(void);
APIEXPORT void APICALL *rest_new(t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL rest_free(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);

APIEXPORT void APICALL rest_command(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec); 
APIEXPORT void APICALL rest_timeout(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec); 
APIEXPORT void APICALL rest_url(t_rest *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [oauth] */
struct _oauth;
typedef struct _oauth t_oauth;

APIEXPORT void APICALL oauth_setup(void);
APIEXPORT void APICALL *oauth_new(t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL oauth_free(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec);

APIEXPORT void APICALL oauth_command(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec); 
APIEXPORT void APICALL oauth_timeout(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL oauth_url(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL oauth_method(t_oauth *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [json-encode] */
struct _json_encode;
typedef struct _json_encode t_json_encode;

APIEXPORT void APICALL setup_json0x2dencode(void);
APIEXPORT void APICALL *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL json_encode_free(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);

APIEXPORT void APICALL json_encode_bang(t_json_encode *x);
APIEXPORT void APICALL json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL json_encode_array_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL json_encode_read(t_json_encode *x, t_symbol *filename);
APIEXPORT void APICALL json_encode_write(t_json_encode *x, t_symbol *filename);
APIEXPORT void APICALL json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [json-decode] */
struct _json_decode;
typedef struct _json_decode t_json_decode;

APIEXPORT void APICALL setup_json0x2ddecode(void);
APIEXPORT void APICALL *json_decode_new(t_symbol *selector, int argcount, t_atom *argvec);

APIEXPORT void APICALL json_decode_string(t_json_decode *x, t_symbol *data);
APIEXPORT void APICALL json_decode_list(t_json_decode *x, t_symbol *selector, int argcount, t_atom *argvec);

/* [urlparams] */
struct _urlparams;
typedef struct _urlparams t_urlparams;

APIEXPORT void APICALL urlparams_setup(void);
APIEXPORT void APICALL *urlparams_new(t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL urlparams_free(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec);

APIEXPORT void APICALL urlparams_bang(t_urlparams *x);
APIEXPORT void APICALL urlparams_add(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec);
APIEXPORT void APICALL urlparams_clear(t_urlparams *x, t_symbol *selector, int argcount, t_atom *argvec);

/* general */ 
APIEXPORT void APICALL purest_json_setup(void);
