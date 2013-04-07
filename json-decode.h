/*
Author:
Thomas Mayer <thomas@residuum.org>

Copyright (c) 2011-2013 Thomas Mayer

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

#include <ctype.h>
#include <string.h>
#include <json/json.h>
#include "purest_json.h"

#ifndef JSON_C_MAJOR_VERSION
#define JSON_C_FIX 1
#elif JSON_C_MAJOR_VERSION < 1 && JSON_MINOR_VERSION <10
#define JSON_C_FIX 1
#else
#define JSON_C_FIX 0
#endif

/* [json-decode] */
struct _json_decode;
typedef struct _json_decode t_json_decode;

APIEXPORT void APICALL *json_decode_new(t_symbol *sel, int argc, t_atom *argv);

APIEXPORT void APICALL json_decode_string(t_json_decode *x, t_symbol *data);
APIEXPORT void APICALL json_decode_list(t_json_decode *x, t_symbol *sel, int argc, t_atom *argv);
