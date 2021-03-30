#!/usr/bin/env bash
#
# Convert a GIMP-generated .xcf file into many .png and .ico files of various sizes.
#

for name in "$@"; do
    infile="$name-512.xcf"
    if [[ -e "$infile" ]]; then
        for size in 512 256 128 64 48 32 16; do
            echo "generating $name-$size.png"
            magick convert "$infile" -background transparent -flatten -resize ${size}x${size} "$name-$size.png"
        done
        echo "generating $name.ico"
        magick convert "$infile" -background transparent -flatten -resize 128x128 "$name.ico"
    else
        echo >&2 "error: $infile not found"
    fi
done
