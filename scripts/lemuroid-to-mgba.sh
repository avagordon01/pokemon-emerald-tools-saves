#!/usr/bin/env bash

set -eux

adb pull "/sdcard/Android/data/com.swordfish.lemuroid/files/saves/" .
adb pull "/sdcard/ROMs/" .
