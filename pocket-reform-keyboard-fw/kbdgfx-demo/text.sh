#!/bin/sh

# Usage: ./text.sh /dev/hidraw3 12 "text as image"

convert -size 128x32 -background black -font "JetBrainsMono-NF-Bold" -pointsize $2 -fill white -gravity center caption:"$3" -depth 8 -flatten gray:temp.bin
sudo ./kbdgfx $1 ./temp.bin
