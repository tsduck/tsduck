## TSDuck documentation files

Subdirectories:

- `user`: TSDuck User Guide (asciidoc).
- `developer`: TSDuck Developer Guide (asciidoc).
- `doxy`: TSDuck Programming Reference (doxygen, from source files).
- `adoc`: Common files for Asciidoctor (CSS, docinfo, PDF theme).

Building the user and developer guides:

- UNIX systems: `make`
- Windows: `.\build-doc.ps1`

In both cases, parameters can be used to:

- generate one of the guides or both,
- generate HTML, PDF, or both,
- open the built guides in browser or PDF reader after generation.

Building the programming reference:

- UNIX systems: `make doxygen`
- Windows: `.\doxy\build-doxygen.ps1`

### Legacy presentations

Various presentations on TSDuck using Microsoft PowerPoint used to be stored here.
They are now moved to the [tsduck-presentations](https://github.com/tsduck/tsduck-presentations) project.

### Asciidoc hints and tips

This section is used to drop various notes on asciidoc format.

Admonition types:  `NOTE`, `TIP`, `IMPORTANT`, `CAUTION`, `WARNING`.
