#!/bin/sh

set -exu

cmake -B build -S. -DFAMILY=rp2040 -DKBD_MODE=KBD_MODE_STANDALONE
cmake --build build
