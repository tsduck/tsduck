//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Input and output plugins for Dektec devices.
//
//----------------------------------------------------------------------------

#include "tsDektecInputPlugin.h"
#include "tsDektecOutputPlugin.h"
#include "tsPluginRepository.h"

// The C++ classes for the plugins are in the shared library libtsdektec.so
// for technical reasons because they reference the binary-only DTAPI. However,
// unlike embedded plugins in libtsduck.so, the Dektec plugin classes do not
// register themselves from the shared library libtsdektec.so. The reason is that
// this shared library is not loaded by default and tsp would not find the plugins.
// This library tsplugin_dektec.so, on the other hand, is fully searched when the
// plugin "dektec" is invoked.

TS_REGISTER_INPUT_PLUGIN(u"dektec", ts::DektecInputPlugin);
TS_REGISTER_OUTPUT_PLUGIN(u"dektec", ts::DektecOutputPlugin);
