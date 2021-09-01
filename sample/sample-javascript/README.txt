This directory contains sample Javascript applications which use the TSDuck library.

The code in class 'SampleTSP' is similar in principle to the C++ sample application
in directory 'sample-app'. It runs a TS processing session with multiple plugins
inside a Java application.

After building TSDuck, it is possible to execute the sample files directly
on the freshly built TSDuck library after executing:

yarn install
yarn link tsduck
node sampleTSP.js
