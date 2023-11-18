**Due to Github's determination to be the "world’s leading AI-powered developer platform" I will move my projects to [Codeberg](https://codeberg.org/Residuum)**
      
      _____        _____  ______  _____ _______        _  _____  ____  _   _
     |  __ \      |  __ \|  ____|/ ____|__   __|      | |/ ____|/ __ \| \ | |
     | |__) |_   _| |__) | |__  | (___    | |         | | (___ | |  | |  \| |
     |  ___/| | | |  _  /|  __|  \___ \   | |     _   | |\___ \| |  | | . ` |
     | |    | |_| | | \ \| |____ ____) |  | |    | |__| |____) | |__| | |\  |
     |_|     \__,_|_|  \_\______|_____/   |_|     \____/|_____/ \____/|_| \_|

PuREST JSON is a library for connecting Puredata (Pd) to HTTP services
and encoding and decoding JSON data.

The library can issue GET, POST, PUT and DELETE statements, so consumation
of RESTful services is possible, e.g. CouchDB.

## About Puredata (From the official website)

Pd (aka Pure Data) is a real-time graphical programming environment for
audio, video, and graphical processing. It is the third major branch
of the family of patcher programming languages known as Max (Max/FTS,
ISPW Max, Max/MSP, jMax, etc.) originally developed by Miller Puckette
and company at IRCAM. The core of Pd is written and maintained by Miller
Puckette and includes the work of many developers, making the whole
package very much a community effort.

## Externals in the library

### `[rest]`
Object for issuing HTTP request.

### `[oauth]`
Object for issuing HTTP requests with OAUTH.

### `[json-encode]`
Object for encoding data to JSON.

### `[json-decode]`
Object for decoding JSON data.

### `[urlparams]`
Object for url encoding and concatenating url parameters.

For the usage of the externals see the help patches for the objects.

## Installation from package repositories

Downloads are available via [deken](https://github.com/pure-data/deken)
which is included in Pd since version 0.47. Packages for Debian and Ubuntu
are available as `pd-purest-json`.

## Downloads of development versions

For each commit, a build in [Circle CI](https://app.circleci.com/pipelines/github/residuum/PuRestJson) 
is triggered. These builds generate deken-like packages, that are available 
from https://cloud.residuum.org/index.php/s/380C60JAabnO7jk

The format for the files is `<Circle-CI-job-number>_<Date-in-YYYY-MM-DD>_<dekenfilename>`.

Those downloads may not work, but represent the current state of development.

## How to build PuREST JSON

The library uses the template for Pd-extended. Drop the library in a
new folder `purest_json` in the "external" path of the Pd-extended
source code and run make in the folder. libcurl and json-c is needed.

Details can be found at
https://github.com/residuum/PuRestJson/wiki/Compilation

If you encouter bugs or feel like a feature is missing, have a look
the bug tracker at
https://github.com/residuum/PuRestJson/issues
