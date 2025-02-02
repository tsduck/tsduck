//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

// Windows mess (again, again, again, ...)
// The object tsComIds in libtscore defines static data for some Windows GUID's
// which are not defined in SDK headers or libraries (well done MS...). When building
// the tscore DLL, we cannot export them because it creates some obscure conflict.
// So, we keep them as internal symbols in the tscore DLL. Here, we are inside the
// tsduck DLL and we can't see internal symbols in the tscore DLL. So, we have to
// redefine all GUID's. Except if we are in the tsduck static library, using the
// tscore static library, because it would create definition conflicts.

#if defined(_TSDUCKDLL_IMPL)
    #define TS_COMIDS_DEFINE 1
    #include "tsComIds.h"
#endif
