#!/usr/bin/env bash

set -eux

rom="Pokemon - Emerald Version (USA, Europe).zip"
srm="Pokemon - Emerald Version (USA, Europe).srm"
sav="Pokemon - Emerald Version (USA, Europe).sav"

adb pull "/storage/self/primary/Android/data/com.swordfish.lemuroid/files/saves/${srm}" .
adb pull "/storage/self/primary/ROMs/${rom}" .

mv "${srm}" "${sav}"
