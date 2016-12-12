#!/bin/bash

brew update
brew install json-c curl liboauth --universal

wget -O /tmp/pd.tar.gz http://msp.ucsd.edu/Software/pd-${PDVERSION}-64bit.mac.tar.gz
tar -xf /tmp/pd.tar.gz -C /tmp

pip install grip beautifulsoup4 lxml
