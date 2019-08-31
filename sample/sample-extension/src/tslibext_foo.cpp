// Sample TSDuck extension.
// This file is included in the shared library.
// It exports the required symbol for this shared library to be recognized as a valid TSDuck extension.

#include "tsduck.h"

// Declare the extension library:
TS_DECLARE_EXTENSION(u"foo", u"Sample foo extension", {u"foot"}, {u"footool"});

// Register our XML file as an extension to the global model for tables and descriptors.
TS_FACTORY_REGISTER_XML(u"tslibext_foo.xml");

// Register our names file as an extension to the global names file.
TS_FACTORY_REGISTER_NAMES(u"tslibext_foo.names");
