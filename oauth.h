#include <stdlib.h>
#include <string.h>
#include <json/json.h>
#include <curl/curl.h>
#include <pthread.h>
#include <oauth.h>

#include "purest_json.h"

struct _oauth;
typedef struct _oauth t_oauth;

struct _rest;
typedef struct _rest t_rest;

APIEXPORT void APICALL *oauth_new(t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL oauth_free(t_oauth *x, t_symbol *sel, int argc, t_atom *argv);

APIEXPORT void APICALL oauth_command(t_oauth *x, t_symbol *sel, int argc, t_atom *argv); 
APIEXPORT void APICALL oauth_timeout(t_oauth *x, t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL oauth_url(t_oauth *x, t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL oauth_method(t_oauth *x, t_symbol *sel, int argc, t_atom *argv);
