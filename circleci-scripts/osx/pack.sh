#!/bin/bash
set -e

bash ./osx_dependencies.sh

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/
python3 create-manual.py

make machine="amd64 arm64" deken.bits=32 deken
