#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <json/json.h>
#include "purest_json.h"

/* [urlparams] */
struct _urlparams;
typedef struct _urlparams t_urlparams;

APIEXPORT void APICALL *urlparams_new(t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL urlparams_free(t_urlparams *x, t_symbol *sel, int argc, t_atom *argv);

APIEXPORT void APICALL urlparams_bang(t_urlparams *x);
APIEXPORT void APICALL urlparams_add(t_urlparams *x, t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL urlparams_clear(t_urlparams *x, t_symbol *sel, int argc, t_atom *argv);
