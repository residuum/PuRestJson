#include "m_pd.h"

#define LIBRARY_VERSION "0.11"

#define REQUEST_TYPE_LEN 7

#ifdef _WIN32
	#define APIEXPORT __declspec(dllexport)
	#define APICALL __cdecl
	#define MYERROR(...) post(__VA_ARGS__)
#else 
	#define APIEXPORT
	#define APICALL
	#define MYERROR(...) error(__VA_ARGS__)
#endif

/* [rest] */
APIEXPORT void APICALL rest_setup(void);

/* [oauth] */
APIEXPORT void APICALL oauth_setup(void);

/* [json-encode] */
APIEXPORT void APICALL setup_json0x2dencode(void);

/* [json-decode] */
APIEXPORT void APICALL setup_json0x2ddecode(void);

/* [urlparams] */
APIEXPORT void APICALL urlparams_setup(void);

/* general */ 
APIEXPORT void APICALL purest_json_setup(void);
