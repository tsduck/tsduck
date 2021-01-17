This directory contains sample Java applications which use the TSDuck library.

The code in class 'SampleTSP' is similar in principle to the C++ sample application
in directory 'sample-app'. It runs a TS processing session with multiple plugins
inside a Java application.

The code in class 'SampleMonitoring' runs two successive TS processing sessions.
The first session downloads a TS file using HTTP and the second one plays the
downloaded file. System monitoring messages are reported to check the stability
of the application.

These two samples programs are functionally identical to the Python samples in
directory ../sample-python.

After building TSDuck, it is possible to execute the Java programs directly
on the freshly built TSDuck library after executing:

- Bash (Linux, macOS):
  source ../../build/setenv.sh
  javac SampleTSP.java
  java SampleTSP

- PowerShell (Windows):
  ..\..\build\setenv.ps1
  javac SampleTSP.java
  java SampleTSP

Some sample applications such as SampleAnalyzeTS need the third party package
org.json to parse the JSON text which is produced by TSDuck. There are scripts
in this directory to download and compile org.json. The resulting JAR file must
be included in the class path.

- Bash (Linux, macOS):
  source ../../build/setenv.sh
  ./get-org-json.sh
  export $CLASSPATH="org.json.jar:$CLASSPATH"
  javac SampleAnalyzeTS.java
  java SampleAnalyzeTS some-ts-file-to-analyze.ts

- PowerShell (Windows):
  ..\..\build\setenv.ps1
  .\get-org-json.ps1 -NoPause
  $env:CLASSPATH="org.json.jar;$env:CLASSPATH"
  javac SampleAnalyzeTS.java
  java SampleAnalyzeTS some-ts-file-to-analyze.ts
