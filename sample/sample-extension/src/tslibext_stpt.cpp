// Sample TSDuck extension.
// This file is included in the shared library.
// It exports the required symbol for this shared library to be recognized as a valid TSDuck extension.

#include "tsduck.h"

TS_DECLARE_EXTENSION(u"stpt",         // extension name
                     u"Handling of Sample Third-Party Table (STPT)",
                     {u"stpt"},       // list of tsp plugins
                     {u"stpttool"});  // list of tools (executables)
