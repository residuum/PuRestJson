#!/bin/bash

brew update
brew install json-c curl liboauth

wget -O /tmp/pd.tar.gz http://msp.ucsd.edu/Software/pd-${PDVERSION}.mac.tar.gz
tar -xf /tmp/pd.tar.gz -C /tmp

pip2 install grip beautifulsoup4 lxml --user
