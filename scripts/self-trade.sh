#!/usr/bin/env bash

set -eux

rom="Pokemon - Emerald Version (USA, Europe).zip"
sav="Pokemon - Emerald Version (USA, Europe).sav"

mkdir -p alt
cp "${rom}" "${sav}" alt

mgba-qt "${rom}"

# File > New multiplayer window
# File > Load ROM...
# open the rom in alt
