## Doxygen documentation

This directory contains the files and scripts to generate the programming
reference documentation, using doxygen, from the source files.

The CSS files are extracted from "Doxygen Awesome", an open-source project
using the MIT license at https://jothepro.github.io/doxygen-awesome-css/

Don't call Doxygen directly. Use the Python script `build-doxygen.py`.
It generates the TSDuck Programming Reference using Doxygen with proper setup.
The file `build-doxygen.ps1` is a PowerShell wrapper for Windows.
