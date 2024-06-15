## Legacy documentation

This directory contains legacy documentation files.
The files in this directory should be considered as read-only.

This is essentially the previous version of the TSDuck User's Guide.
This guide was edited using Microsoft Word. By nature, it is frequently
updated, for each new option in a command or plugin, for each new table
or descriptor. Since binary files such as `.pptx` and `.pdf` are stored
as complete files in the git directory, the history of the repo became
huge (1.8 GB), mostly because of these files.

The decision was made to migrate the user's guide to asciidoc format.
The source files are text files which are perfectly managed by git. The
PDF can be easily regenerated using scripts and no longer need to be kept
in the repo. Finally, the generated HTML files are high quality.

After this first step, the history of the repo will no longer grow as
badly as before. However, the huge history remains. At some point in the
near future, we will "rewrite history" of the repo to get rid of all
previous versions of these files, keeping only the last version in this
directory. However, this will rewrite the commit history and invalidate
all forked repositories. Because of these consequences on contributors,
the operation is delayed after thorough testing.

### Font files

The files `gadugi.ttf` and `gadugib.ttf` contain the Gadugi true-type font,
used in the documents.

### Scripts

The PowerShell script `build-user-guide-pdf.ps1` was used to update `tsduck.docx`
and generate `tsduck.pdf` from `tsduck.docx`.

It updates the TSDuck version, current month, table of contents, etc. The idea
is to manually update `tsduck.docx` with new content using Word, then exit and
run `build-user-guide-pdf.ps1` to update the structure and version of the document.

This script is kept here for reference only.
