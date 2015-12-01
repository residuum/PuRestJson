## based on Pd library template version 1.0.14
# For instructions on how to use this template, see:
#  http://puredata.info/docs/developer/MakefileTemplate
lib.name = purest_json

# objects to build from .c files in src/
OBJECTS = rest oauth json-decode json-encode urlparams 

# list all pd objects (i.e. myobject.pd) files here, and their helpfiles will
# be included automatically
PDOBJECTS = 

# example patches and related files, in the 'examples' subfolder
EXAMPLES = purest-json-test.pd the-sound-of-money.pd statistics.pd twitter-client.pd binary-test.pd

# if you want to include any other files in the source and binary tarballs,
# list them here.  This can be anything from header files, test patches,
# documentation, etc. 
EXTRA_DIST = README.md LICENSE.txt Changelog.txt test.json

HELPPATCHES = json-help.pd urlparams-help.pd rest-help.pd

# unit tests and related files here, in the 'unittests' subfolder
UNITTESTS = 

LIBRARY_VERSION = $(shell sed -n 's|^\#X text [0-9][0-9]* [0-9][0-9]* VERSION \(.*\);|\1|p' $(lib.name)-meta.pd)

datafiles = $(addprefix examples/, $(EXAMPLES)) $(EXTRA_DIST) $(HELPPATCHES)
class.sources = $(addprefix src/, $(OBJECTS))
cflags += -DPD -DVERSION='"$(LIBRARY_VERSION)"'
ldflags = 
ldlibs = -lcurl -ljson-c -loauth

include pd-lib-builder/Makefile.pdlibbuilder
