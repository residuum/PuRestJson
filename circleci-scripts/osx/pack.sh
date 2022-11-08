#!/bin/bash
set -e

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python3 create-manual.py

bash ./osx_dependencies.sh
make machine="amd64 arm64" deken.bits=32 deken
