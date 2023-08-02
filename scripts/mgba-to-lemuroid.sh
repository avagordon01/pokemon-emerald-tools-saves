#!/usr/bin/env bash

set -eux

srm="Pokemon - Emerald Version (USA, Europe).srm"
sav="Pokemon - Emerald Version (USA, Europe).sav"

#mgba appends 32 bytes for RTC to the end of the 128KB save (lemuroid doesn't like this)
#time-based events are a tiny part of emerald, so it might be fine to truncate this RTC footer
#https://bulbapedia.bulbagarden.net/wiki/Time#Generation_III
head -c 131072 "${sav}" > "${srm}"

adb push "${srm}" "/storage/self/primary/Android/data/com.swordfish.lemuroid/files/saves"
