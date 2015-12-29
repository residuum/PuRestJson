lib.name = purest_json

OBJECTS = rest oauth json-decode json-encode urlparams 
PDOBJECTS = 
EXAMPLES = purest-json-test.pd the-sound-of-money.pd statistics.pd twitter-client.pd binary-test.pd
EXTRA_DIST = README.md LICENSE.txt Changelog.txt test.json
HELPPATCHES = json-help.pd urlparams-help.pd rest-help.pd
UNITTESTS = 

datafiles = $(addprefix examples/, $(EXAMPLES)) $(EXTRA_DIST) $(HELPPATCHES)
class.sources = $(addprefix src/, $(OBJECTS))

cflags += -DPD -DVERSION='"$(lib.version)"' -std=c99 
ldflags = 
ldlibs = -lcurl -ljson-c -loauth

define forWindows
    ldlibs += -lpthread -lm -lidn -lintl -lwldap32 -lgnutls -lhogweed -lgmp -lssl \
		-liconv -lnettle -lssh2 -lgcrypt -lgpg-error -lcrypto \
		-lws2_32 -lgdi32 -lcrypt32 -lz  
    cflags += -mthreads -DCURL_STATICLIB
endef

lib.setup.sources = src/purest_json

include pd-lib-builder/Makefile.pdlibbuilder
