#!/bin/bash

wget https://curl.haxx.se/ca/cacert.pem
bash ./embed-osx-dep-homebrew.sh

make deken
