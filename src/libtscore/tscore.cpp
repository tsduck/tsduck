//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

// This module is only here to include all libtscore header files. This is
// done on purpose to make sure that all inlined functions are compiled at
// least once. Otherwise, on Windows, the libtscore DLL will not contain the
// referenced code.

#include "tscore.h"
