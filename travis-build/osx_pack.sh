#!/bin/bash

wget https://curl.haxx.se/ca/cacert.pem
bash ./osx_dependencies.sh
make deken
