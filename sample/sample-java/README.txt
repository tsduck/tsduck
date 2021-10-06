This directory contains sample Java applications which use the TSDuck library.

The code in class 'SampleTSP' is similar in principle to the C++ sample application
in directory 'sample-app'. It runs a TS processing session with multiple plugins
inside a Java application.

Other sample programs illustrate other features. The file japanese-tables.bin
contains binary tables and is used as input by SampleJapaneseTables.java.

These samples programs are functionally identical to the Python samples in
directory ../sample-python.

After building TSDuck, it is possible to execute the Java programs directly
on the freshly built TSDuck library after executing:

- Bash (Linux, macOS):
  source ../../scripts/setenv.sh
  javac SampleTSP.java
  java SampleTSP

- PowerShell (Windows):
  ..\..\scripts\setenv.ps1
  javac SampleTSP.java
  java SampleTSP

Some sample applications such as SampleAnalyzeTS need the third party package
org.json to parse the JSON text which is produced by TSDuck. There are scripts
in this directory to download and compile org.json. The resulting JAR file must
be included in the class path.

- Bash (Linux, macOS):
  source ../../scripts/setenv.sh
  ./get-org-json.sh
  export CLASSPATH="org.json.jar:$CLASSPATH"
  javac SampleAnalyzeTS.java
  java SampleAnalyzeTS some-ts-file-to-analyze.ts

- PowerShell (Windows):
  ..\..\scripts\setenv.ps1
  .\get-org-json.ps1 -NoPause
  $env:CLASSPATH="org.json.jar;$env:CLASSPATH"
  javac SampleAnalyzeTS.java
  java SampleAnalyzeTS some-ts-file-to-analyze.ts
