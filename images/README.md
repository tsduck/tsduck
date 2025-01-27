# TSDuck image files

## TSDuck logo

The TSDuck logo is editable in `tsduck-512.xcf` (GIMP file) with a size of 512x512.
Version of different sizes are exported as `.png` files.

One `.ico` file was exported and is used in resource files on Windows so that
TSDuck executables and DLL's are marked with the TSDuck logo.

The variants named `tsduck-extension` are proposed for TSDuck extension.
This is the same logo as TSDuck to emphasize the relationship with the parent project,
with a Persian Orange variation to outline the independence from TSDuck.

## Color palette in images, documentation and web site

The color palette is rendered in `palette.xcf` (GIMP file) and `palette.png`.

The palette shall be consistently used in the documentation, the diagrams, the web site tsduck.io.

| Name              | MS Office | Decimal RGB   | Hexa RGB |
| ----------------- | --------- | ------------- | -------- |
| Dark Spring Green | Accent 1  |  44, 110,  73 | 2C6E49   |
| Middle Green      | Accent 2  |  76, 149, 108 | 4C956C   |
| Light Yellow      | Accent 3  | 254, 254, 227 | FEFEE3   |
| Melon             | Accent 4  | 255, 201, 185 | FFC9B9   |
| Persian Orange    | Accent 5  | 214, 140,  69 | D68C45   |
| Light Green       | Accent 6  | 133, 193, 159 | 85C19F   |
| Teal Green        |           |   0, 150, 136 | 009688   |

## Diagrams for documentation, presentation, and web site

The diagrams are created using Microsoft PowerPoint and exported as `.png` files.

Initially, there was one big documentation file. Now, it has been split into
one smaller file per diagram.

## SVG files

The XML contents of SVG files were edited by hand.

Using `<text>` SVG tags with imported fonts works on most browsers but not with GIMP.
Therefore, the name "TSDuck" was converted to a SVG path using https://danmarshall.github.io/google-font-to-svg-path/
The font file `gadugib.ttf` was uploaded.
