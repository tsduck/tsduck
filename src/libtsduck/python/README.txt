This directory contains Python bindings to some TSDuck features.

See the file src/doc/python-bindings.md for more details on how
to use them from Python applications.

Subdirectories:

- ts : Python code to export to applications. The name of the TSDuck Python
  module is consequently "ts", the same as the TSDuck C++ namespace.

- private: C++ implementation of the Python bindings, included in the TSDuck
  binary library.
