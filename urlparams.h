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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "purest_json.h"

/* suppresses warning, nothing special */
#define NO_BACKSLASHES 1

/* [urlparams] */
struct _urlparams;
typedef struct _urlparams t_urlparams;

/* constructor */
APIEXPORT void APICALL *urlparams_new(const t_symbol *sel, const int argc, const t_atom *argv);
/* destructor */
APIEXPORT void APICALL urlparams_free(t_urlparams *x, const t_symbol *sel, const int argc, const t_atom *argv);

/* bang and output */
APIEXPORT void APICALL urlparams_bang(t_urlparams *x);
/* add value */
APIEXPORT void APICALL urlparams_add(t_urlparams *x, const t_symbol *sel, const int argc, t_atom *argv);
/* clear stored values */
APIEXPORT void APICALL urlparams_clear(t_urlparams *x, const t_symbol *sel, const int argc, const t_atom *argv);
