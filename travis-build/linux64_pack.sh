#!/bin/bash

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python create-manual.py

make machine=x86_64 deken
