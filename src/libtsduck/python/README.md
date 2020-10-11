## TSDuck Python Bindings

This directory contains Python bindings to some TSDuck features.

### Scope

Although subject to evolution, these bindings will never aim at
supporting the full TSDuck feature set since this would be too large.
Only a small subset of TSDuck high-level features are targeted.

It is currently in experimental stage only, not yet documented and
subject to radical change in the near future. The TSDuck author being
much less fluent in Python than C++, the code is probably far from the
"pythonic way" but is willing to evolve toward it.

### Python prerequisites

The code was tested with Python 3.7 and higher. Python 2.x is not supported.
Intermediate versions may work but without guarantee.

### Implementation

There are usually two ways to call C/C++ from Python:

- Using the predefined `ctypes` Python module to call C functions,
- Implementating a full native Python module in C/C++.

The second option is usually more flexible and more generic. However,
the generated binary depends on the version of Python. If such an option
is used, the binary installation of TSDuck would require a specific version
of Python (or a specific set of versions of it). But each system has it own
requirements on Python and it is difficult for a product like TSDuck to
impose a specific version of Python.

Consequently, the less flexible `ctypes` approach was chosen. The TSDuck
binary library contains C wrapper functions to some features of TSDuck and
these carefully crafted functions are directly called from Python code
using `ctypes`, regardless of the version of Python.

### Subdirectories

- `ts` : Python code to export to applications. The name of the TSDuck Python
  module is consequently `ts`, the same as the TSDuck C++ namespace.
- `private`: C++ implementation of the Python bindings, included in the
  TSDuck binary library.
