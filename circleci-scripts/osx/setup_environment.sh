#!/bin/bash

brew update
brew install json-c curl liboauth

curl -o /tmp/pd.tar.gz http://msp.ucsd.edu/Software/pd-${PDVERSION}-macosx7.mac.tar.gz
tar -xf /tmp/pd.tar.gz -C /tmp

pip3 install grip beautifulsoup4 lxml --user
