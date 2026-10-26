#!/bin/sh

set -exu

cmake -B build -S. -DFAMILY=rp2040 -DKBD_MODE=KBD_MODE_LAPTOP
cmake --build build
