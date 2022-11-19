TSDuck documentation files

- tsduck.docx tsduck.pdf
TSDuck user's guide (Microsoft Word).

- build-user-guide-pdf.ps1
PowerShell script (Windows) to update tsduck.docx and tsduck.pdf from tsduck.docx.
Update the TSDuck version, current month, table of contents, etc. The idea is to
manually update tsduck.docx with new content using Word, then exit and run
build-user-guide-pdf.ps1 to update the structure and version of the document.

- tsduck-diagrams.pptx
PowerPoint document containing all diagrams, copied into other documents.

- tsduck-presentation.pptx tsduck-presentation.pdf
TSDuck presentation (Microsoft PowerPoint).

- tsduck-ibc2018.pptx tsduck-ibc2018.pdf
TSDuck presentation at IBC 2018 (Microsoft PowerPoint). Obsolete, for reference only.

- mpegts-introduction.pptx mpegts-introduction.pdf
An introduction to MPEG-TS (Microsoft PowerPoint).

- tsduck-project.pptx tsduck-project.pdf
TSDuck, "anatomy of a single-person open-source project" (Microsoft PowerPoint).

- tsduck-coding.docx tsduck-coding.pdf
TSDuck coding guidelines.

- japan-sat.xlsx japan-sat.pdf
Satellite frequencies in Japan (Microsoft Excel).

- gadugi.ttf gadugib.ttf
Gadugi true-type font, used in documents.

- Doxyfile doxygen-awesome.css doxygen-awesome-sidebar-only.css
Configuration files, used to generate TSDuck programmer's guide (Doxygen).
The CSS files are extracted from "Doxygen Awesome", an open-source project
using the MIT license at https://jothepro.github.io/doxygen-awesome-css/

- build-doxygen.sh build-doxygen.ps1
Shell script (Linux, macOS), PowerShell script (Windows) to generate TSDuck
programmer's guide (Doxygen).
