#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json/json.h>
#include <pthread.h>
#include <oauth.h>
#include "purest_json.h"

struct _rest;
typedef struct _rest t_rest;

APIEXPORT void APICALL *rest_new(t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL rest_free(t_rest *x, t_symbol *sel, int argc, t_atom *argv);

APIEXPORT void APICALL rest_command(t_rest *x, t_symbol *sel, int argc, t_atom *argv); 
APIEXPORT void APICALL rest_timeout(t_rest *x, t_symbol *sel, int argc, t_atom *argv); 
APIEXPORT void APICALL rest_url(t_rest *x, t_symbol *sel, int argc, t_atom *argv);
