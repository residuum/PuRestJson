  _____        _____  ______  _____ _______        _  _____  ____  _   _ 
 |  __ \      |  __ \|  ____|/ ____|__   __|      | |/ ____|/ __ \| \ | |
 | |__) |_   _| |__) | |__  | (___    | |         | | (___ | |  | |  \| |
 |  ___/| | | |  _  /|  __|  \___ \   | |     _   | |\___ \| |  | | . ` |
 | |    | |_| | | \ \| |____ ____) |  | |    | |__| |____) | |__| | |\  |
 |_|     \__,_|_|  \_\______|_____/   |_|     \____/|_____/ \____/|_| \_|
                                                                         

PuREST JSON is a library for connecting Puredata (PD) to HTTP services 
and encoding and decoding JSON data.

The library can issue GET, POST, PUT and DELETE statements, so consumation
of RESTful services is possible, e.g. CouchDB. 

The library used to be called CouchPdb, but handles all HTTP requests,
so the name was not fitting any more.


About Puredata (From the official website)

Pd (aka Pure Data) is a real-time graphical programming environment for
audio, video, and graphical processing. It is the third major branch
of the family of patcher programming languages known as Max (Max/FTS,
ISPW Max, Max/MSP, jMax, etc.) originally developed by Miller Puckette
and company at IRCAM. The core of Pd is written and maintained by Miller
Puckette and includes the work of many developers, making the whole
package very much a community effort.


Externals in the library

[rest-json]
Object for issuing HTTP request. Received data is parsed as JSON data. 

[json-encode]
Object for encoding data to JSON.

[json-decode]
Object for decoding JSON data.

For the usage of the externals see the help patches for the objects.


How to build PuREST JSON 

The library uses the template for Pd-extended. Drop the library in a
new folder "purest_json" in the "external" path of the Pd-extended
source code and run make in the folder. libcurl and json-c is needed.

Here are the steps in detail:
1. Download the sources of Pd-extended from 
   http://puredata.info/community/projects/software/pd-extended

2. Create a subfolder in the extracted source Pd-0.42.5-extended/externals 
   with the name purest_json and gut the sources of purest_json in there.

3. Make sure to have the header files for libcurl and json-c installed, 
   on Debian the libraries are called libcurl-dev (with several packages 
   providing this virtual package) and libjson0-dev respectively. 

4. Now run "make" inside the folder Pd-0.42.5-extended/externals/purest_json/
   and you should get a file purest_json.EXT, where EXT is the library file
   extension for your platform (dll, pd_darwin, pd_linux). The help patches
   are the *.pd files in the root folder.

5. Some example patches are in the examples folder.


TODO:
- Implementing OAUTH
- JSON error when querying Twitter
