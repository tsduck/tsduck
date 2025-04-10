## TSDuck documentation files

Subdirectories:

- `user`: TSDuck User's Guide (asciidoc).
- `developer`: TSDuck Developer's Guide (asciidoc).
- `doxy`: TSDuck Programming Reference (doxygen, from source files).
- `adoc`: Common files for Asciidoctor (CSS, docinfo, PDF theme).
- `presentations`: Various presentations on TSDuck (Microsoft Office).

Building the user and developer's guides:

- UNIX systems: `make`
- Windows: `.\build-doc.ps1`

In both cases, parameters can be used to:

- generate one of the guides or both,
- generate HTML, PDF, or both,
- open the built guides in browser or PDF reader after generation.

Building the programming reference:

- UNIX systems: `make doxygen`
- Windows: `.\doxy\build-doxygen.ps1`

### Asciidoc hints and tips

Admonition types:  `NOTE`, `TIP`, `IMPORTANT`, `CAUTION`, `WARNING`.
