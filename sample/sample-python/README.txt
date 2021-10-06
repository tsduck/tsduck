This directory contains sample Python applications which use the TSDuck library.

The code in 'sample-tsp.py' is similar in principle to the C++ sample application
in directory 'sample-app'. It runs a TS processing session with multiple plugins
inside a Python application.

Other sample programs illustrate other features. The file japanese-tables.bin
contains binary tables and is used as input by sample-japanese-tables.py.

These samples programs are functionally identical to the Java samples in
directory ../sample-java.

After building TSDuck, it is possible to execute the Python programs directly
on the freshly built TSDuck library after executing:

- Bash (Linux, macOS):
  source ../../scripts/setenv.sh
  ./sample-tsp.py

- PowerShell (Windows):
  ..\..\scripts\setenv.ps1
  .\sample-tsp.py
