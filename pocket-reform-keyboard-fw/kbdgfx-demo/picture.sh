#!/bin/sh

# Usage: ./picture.sh /dev/hidraw3 picture.png

convert $2 -resize 128 -gravity center -extent 128x32 -depth 8 gray:temp.bin
sudo ./kbdgfx $1 ./temp.bin
