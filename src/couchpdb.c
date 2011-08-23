#include "couchpdb.h"
#include "couchdb.c"
#include "json.c"

void couchpdb_setup(void) {
	couchdb_class = class_new(gensym("couchdb"), (t_newmethod)couchdb_new,
			0, sizeof(t_couchdb), 0, A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_oauth, gensym("oauth"), A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_url, gensym("url"), A_GIMME, 0);
	class_addmethod(couchdb_class, (t_method)couchdb_command, gensym("couchdb"), A_GIMME, 0);

	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
		0, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
}
