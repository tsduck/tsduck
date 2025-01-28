# TSDuck image files

## TSDuck logo

The TSDuck logo is editable in `tsduck-logo.svg` (Scalable Vector Graphics).
No particular graphics editor was used. SVG is simply XML text with graphics directives
and the logo is simple enough to be hand-crafted using a text editor.

The SVG file is designed with a size of 1000x1000 but its structure is scalable by design.
Version of different sizes are exported as `.png` files. One `.ico` file was exported and
is used in resource files on Windows so that TSDuck executables and DLL's are marked with
the TSDuck logo.

The variants named `tsduck-extension` are proposed for TSDuck extension.
This is the same logo as TSDuck to emphasize the relationship with the parent project,
with a Persian Orange variation to outline the independence from TSDuck.

The PNG files of various sizes and the `.ico` files are generated using ImageMagick in
the script `convert-logo.sh`. However, ImageMagick is not able to correctly render the
transparent background of SVG files. Therefore, GIMP was used to manually export the
512x512 version of the PNG. Then, the script generates all other versions from this
largest PNG file.

## Color palette in images, documentation and web site

The color palette is rendered in `palette.svg` and `palette.png`.

The palette shall be consistently used in the logo, documentation, the diagrams,
and the web site tsduck.io.

| Name              | Decimal RGB   | Hexa RGB |
| ----------------- | ------------- | -------- |
| Dark Spring Green |  44, 110,  73 | 2C6E49   |
| Middle Green      |  76, 149, 108 | 4C956C   |
| Light Green       | 133, 193, 159 | 85C19F   |
| Light Yellow      | 254, 254, 227 | FEFEE3   |
| Melon             | 255, 201, 185 | FFC9B9   |
| Persian Orange    | 214, 140,  69 | D68C45   |
| Teal Green        |   0, 150, 136 | 009688   |

## Diagrams for documentation, presentation, and web site

The diagrams are created using Microsoft PowerPoint and exported as `.png` files.

Initially, there was one big documentation file. Now, it has been split into
one smaller file per diagram.

## SVG files

The XML contents of SVG files were edited by hand.

Using `<text>` SVG tags with imported fonts works on most browsers but not with GIMP,
Emacs, or ImageMagick. In practice, the SVG format is correctly handled in a few tools
only. Therefore, in the logo, the name "TSDuck" was converted to a SVG path using
https://danmarshall.github.io/google-font-to-svg-path/
The font file `gadugib.ttf` was uploaded to draw the text as a SVG path.

In the palette, the font rendering is not important and plain text is used in the SVG
file. Conversion to PNG was done once using GIMP.
