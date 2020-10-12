This directory contains sample Python applications which use the TSDuck library.

The code in 'sample-tsp.py' is similar in principle to the C++ sample application
in directory 'sample-app'. It runs a TS processing session with multiple plugins
inside a Python application.

The code in 'sample-monitoring.py' runs two successive TS processing sessions.
The first session downloads a TS file using HTTP and the second one plays the
downloaded file. System monitoring messages are reported to check the stability
of the application.

After building TSDuck, it is possible to execute the Python programs directly
on the freshly built TSDuck library after executing:

- Bash (Linux, macOS):
  source ../../build/setenv.sh
  ./sample-tsp.py

- PowerShell (Windows):
  ..\..\build\setenv.ps1
  .\sample-tsp.py
