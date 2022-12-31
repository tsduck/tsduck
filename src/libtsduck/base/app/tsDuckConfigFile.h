//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  A singleton which contains the TSDuck configuration file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsConfigFile.h"
#include "tsSingletonManager.h"

namespace ts {
    //!
    //! A singleton which contains the TSDuck configuration file.
    //! @ingroup app
    //!
    class TSDUCKDLL DuckConfigFile : public ConfigFile
    {
        // This class is a singleton. Use static Instance() method.
        TS_DECLARE_SINGLETON(DuckConfigFile);

    public:
        //!
        //! Get the value of an entry.
        //! A section with the name of the executable is searched first.
        //! Then, the global section is used.
        //! @param [in] entry Entry name.
        //! @param [in] defvalue Default value.
        //! @return The value in the entry or @a defvalue if @a entry does not exist.
        //!
        UString value(const UString& entry, const UString& defvalue = UString()) const;

        //!
        //! Get all values of an entry.
        //! A section with the name of the executable is searched first.
        //! Then, the global section is used.
        //! @param [in] entry Entry name.
        //! @param [out] values Vector of values.
        //!
        void getValues(const UString& entry, UStringVector& values) const;

    private:
        const UString        _appName;
        const ConfigSection& _appSection;
        const ConfigSection& _mainSection;
    };
}
