//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Build version string
//
//----------------------------------------------------------------------------

#include "tsVersion.h"
#include "tsFormat.h"
#include "tsToInteger.h"
#include "tsSysUtils.h"
#include "tsSharedLibrary.h"
#include "tsDektecUtils.h"


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

std::string ts::GetVersion(VersionFormat format, const std::string& applicationName)
{
    const std::string version(TS_STRINGIFY(TS_VERSION_MAJOR) "." TS_STRINGIFY(TS_VERSION_MINOR));
    switch (format) {
        case VERSION_LONG:
            return (applicationName.empty() ? "" : applicationName + ": ") +
                "TSDuck - The MPEG Transport Stream Toolkit "
            #if defined(TS_NO_DTAPI)
                "(no Dektec support) "
            #endif
                "- version " + version;
        case VERSION_NSIS:
            return "!define tsduckVersion \"" + version + '"';
        case VERSION_DEKTEC:
            return GetDektecVersions();
        default: // includes VERSION_SHORT
            return version;
    }
}
