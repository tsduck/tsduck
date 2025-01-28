#!/usr/bin/env bash
#
# Convert the 512x512 PNG version of the logos to all sizes.
#
# Note: ImageMagick 7 is not able to convert from the SVG files: some paths are
# not visible, the background is white instead of transparent, etc. We manually
# convert the SVG files into the 512x512 PNG version using GIMP first. Then, we
# convert this large PNG into all other sizes using ImageMagick.
#

convert-file() {
    infile="$1"
    outprefix="$2"
    if [[ -e "$infile" ]]; then
        for size in 256 128 64 48 32 16; do
            echo "generating $outprefix-$size.png"
            magick "$infile" -background none -flatten -resize ${size}x${size} "$outprefix-$size.png"
        done
        echo "generating $outprefix.ico"
        magick "$infile" -background none -flatten -resize 128x128 "$outprefix.ico"
    else
        echo >&2 "error: $infile not found"
    fi
}

convert-file tsduck-512.png tsduck
convert-file tsduck-extension-512.png tsduck-extension
