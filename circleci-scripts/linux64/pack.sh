#!/bin/bash
set -e
bash ./linux_dependencies.sh *.pd_linux

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python3 create-manual.py

make machine=amd64 deken.bits=32 deken
