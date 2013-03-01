#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json/json.h>
#include "purest_json.h"

/* [json-encode] */
struct _json_encode;
typedef struct _json_encode t_json_encode;

APIEXPORT void APICALL *json_encode_new(t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL json_encode_free(t_json_encode *x, t_symbol *sel, int argc, t_atom *argv);

APIEXPORT void APICALL json_encode_bang(t_json_encode *x);
APIEXPORT void APICALL json_encode_add(t_json_encode *x, t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL json_encode_array_add(t_json_encode *x, t_symbol *sel, int argc, t_atom *argv);
APIEXPORT void APICALL json_encode_read(t_json_encode *x, t_symbol *filename);
APIEXPORT void APICALL json_encode_write(t_json_encode *x, t_symbol *filename);
APIEXPORT void APICALL json_encode_clear(t_json_encode *x, t_symbol *sel, int argc, t_atom *argv);
