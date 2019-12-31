//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Artificial references to all DVB items, for use with static library.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Artificial references to all DVB items.
    //! @ingroup app
    //!
    //! This class is useful only when using the TSDuck static library.
    //! It is useless, although harmless, when using the TSDuck DLL or shared library.
    //!
    //! Within the TSDuck library, DVB classes like tables, descriptors or
    //! character sets register themselves in a factory system when their
    //! object module is initialized. All these modules are in the TSDuck
    //! library. When using the TSDuck DLL (Windows) or shared library (UNIX),
    //! all these DVB object modules are automatically included and registered.
    //!
    //! However, when using the TSDuck static library, no direct reference
    //! exist to these modules and they are not included in the final executable.
    //! As a consequence, most DVB tables, descriptors or character sets are
    //! not recognized when present in transport streams.
    //!
    //! This file creates dummy references to all known DVB classes which
    //! register themselves without other explicit reference. By referencing
    //! this file from the application, all DVB classes are forced into the
    //! final executable. Again, this is required only when linking against
    //! the TSDuck static library.
    //!
    //! This class does nothing. But creating an instance of it automatically
    //! pulls references to all self-registered DVB classes:
    //!
    class TSDUCKDLL StaticReferencesDVB
    {
    public:
        //!
        //! Default constructor.
        //!
        StaticReferencesDVB();
    private:
        std::vector<const void*> _refs;
    };
}
