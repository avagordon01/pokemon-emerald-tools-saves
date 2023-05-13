#!/usr/bin/env bash

set -eux

srm="Pokemon - Emerald Version (USA, Europe).srm"
sav="Pokemon - Emerald Version (USA, Europe).sav"

# mgba appends 32 unknown bytes to the end of the 128KB save
head -c 131072 "${sav}" > "${srm}"

adb push "${srm}" "/storage/self/primary/Android/data/com.swordfish.lemuroid/files/saves"
