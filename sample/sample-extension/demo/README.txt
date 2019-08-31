This directory contains scripts to demonstrate the TSDuck "foo" extension.

A fake transport stream containing one service is created by the script
build-stream.sh. A complete PSI/SI structure is built using XML files
and fake data streams replace audio and video. A Foo Table and a couple
of foo_descriptors are built, demonstrating the ability to declare
additional tables and descriptors using XML models.

The transport stream is then analyzed by the same scripts, demonstrating
the ability to analyze tables, descriptors and CAS data from an extension.
