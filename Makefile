lib.name = purest_json

OBJECTS = rest.c oauth.c json-decode.c json-encode.c urlparams.c 
PDOBJECTS = 
EXAMPLES = purest-json-test.pd the-sound-of-money.pd twitter-client.pd binary-test.pd
EXTRA_DIST = README.md LICENSE.txt Changelog.txt test.json
HELPPATCHES = json-help.pd urlparams-help.pd rest-help.pd
UNITTESTS = 

datafiles = $(EXTRA_DIST) $(HELPPATCHES)
examplefiles = $(addprefix examples/, $(EXAMPLES))
class.sources = $(addprefix src/, $(OBJECTS))
uthash = src/uthash/src

# creating deken package
deken.bits = 32
deken.ext = dek
deken.pack = zip -9 -r
deken.tmp = deken-tmp
deken.folder = $(lib.name)
deken.dependencies =

multi_cflags =
ifeq ($(multi), true)
    multi_cflags = -DPDINSTANCE
endif

cflags += -DVERSION='"$(lib.version)"' -I"$(uthash)" -std=c99 $(multi_cflags)
ldflags = 
ldlibs = -lcurl -ljson-c -loauth

define forWindows
    ldlibs += -lpthread -lm -lwldap32 -lssl -lssh2 -lbcrypt -lgcrypt -lgpg-error \
			  -lcrypto -lws2_32 -lgdi32 -lcrypt32 -lz -lidn2 -lunistring \
			  -latomic -lintl -liconv -lcharset
    cflags += -mthreads -DCURL_STATICLIB
endef

define forDarwin
    ldflags = $(patsubst %,-L%,$(wildcard /opt/homebrew/lib))
    deken.dependencies = *.dylib
endef

define forLinux
    deken.dependencies = *.so*
endef

lib.setup.sources = src/purest_json.c

PDLIBBUILDER_DIR=pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

space := $(subst ,, )
deken.systems := $(foreach m,$(machine),($(system)-$(m)-$(deken.bits)))
deken.systemscleaned := $(subst $(space),,$(deken.systems))
deken.file := $(lib.name)[v$(lib.version)]$(deken.systemscleaned)

deken:
	mkdir -p "$(deken.tmp)"
	mkdir -p "$(deken.tmp)/$(deken.folder)"
	cp $(executables) $(deken.dependencies) "$(deken.tmp)/$(deken.folder)/"
	cp $(datafiles) "$(deken.tmp)/$(deken.folder)/"
	mkdir -p "$(deken.tmp)/$(deken.folder)/examples"
	cp -r manual "$(deken.tmp)/$(deken.folder)/"
	cp $(examplefiles) "$(deken.tmp)/$(deken.folder)/examples"
	cd "$(deken.tmp)"; \
	  $(deken.pack) "$(deken.file).$(deken.ext)" "$(deken.folder)"; \
	  rm -rf "$(deken.folder)"; \
	  mv "$(deken.file).$(deken.ext)" ..; \

clobber: clean
	rm -f $(deken.dependencies)
