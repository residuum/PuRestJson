#!/bin/bash
set -e

brew update
brew install json-c curl liboauth

curl -o /tmp/pd.zip https://msp.puredata.info/Software/pd-${PDVERSION}.macos.zip
unzip -q -d /tmp /tmp/pd.zip
sudo hdiutil attach /tmp/Pd-${PDVERSION}.dmg

pip3 install grip beautifulsoup4 lxml --user
