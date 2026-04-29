#!/bin/sh

# Usage: ./picture-floyd.sh /dev/hidraw3 picture.jpg

convert $2 -resize 128 -gravity center -extent 128x32 -depth 8 -dither FloydSteinberg -remap pattern:gray50 gray:temp.bin
sudo ./kbdgfx $1 ./temp.bin
