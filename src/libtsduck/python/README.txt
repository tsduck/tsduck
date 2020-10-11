This directory contains Python bindings to some TSDuck features.

Although subject to evolution, these bindings will never aim at
supporting the full TSDuck feature set since this would be too large.
Only a small subset of TSDuck high-level features are targeted.

It is currently in experimental stage only, not yet documented and
subject to radical change in the near future.

Python prerequisites: The code was tested with Python 3.7 and higher.
It is likely to work with 3.3 and higher. Python 2.x is not supported.

Implementation: There are usually two ways to call C/C++ from Python:
1) using the predefined "ctypes" Python module to call C functions,
2) implementation a full native Python module in C/C++.
The second option is usually more flexible and more generic. However,
the generated binary depends on the version of Python. If such an option
is used, the binary installation of TSDuck would require a specific
version of Python. But each system has it own requirements on Python
and it is difficult for a product like TSDuck to impose a specific
version of Python. Consequently, the less flexible "ctypes" approach
was chosen. The TSDuck binary library contains C wrapper functions to
some features of TSDuck and these carefully crafted functions are directly
called from Python code using "ctypes", regardless of the version of Python.

Subdirectories:
- ts : Python code to export to applications. The name of the Python
  module is then "ts".
- private: C++ implementation of the Python bindings, included in the
  TSDuck binary library.
