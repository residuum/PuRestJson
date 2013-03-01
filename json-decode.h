#include <ctype.h>
#include <string.h>
#include <json/json.h>
#include "purest_json.h"

/* [json-decode] */
struct _json_decode;
typedef struct _json_decode t_json_decode;

APIEXPORT void APICALL *json_decode_new(t_symbol *sel, int argc, t_atom *argv);

APIEXPORT void APICALL json_decode_string(t_json_decode *x, t_symbol *data);
APIEXPORT void APICALL json_decode_list(t_json_decode *x, t_symbol *sel, int argc, t_atom *argv);
