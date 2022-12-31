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
//!  Abstract interface displaying an object.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsReport.h"

namespace ts {
    //!
    //! An interface to be implemented by classes supporting display to a standard text stream.
    //! @ingroup cpp
    //!
    class TSDUCKDLL DisplayInterface
    {
    public:
        //!
        //! Display the content of this object to a stream.
        //! @param [in,out] stream The stream where to print the content. Standard output by default.
        //! @param [in] margin The prefix string on each line, empty by default.
        //! @param [in] level Severity level (for instance, Severity::Info or Severity::Debug may display more information).
        //! @return A reference to @a stream.
        //!
        virtual std::ostream& display(std::ostream& stream = std::cout, const UString& margin = UString(), int level = Severity::Info) const = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~DisplayInterface();
    };
}

//!
//! Display operator for displayable objects.
//! @param [in,out] strm Where to output the content.
//! @param [in] obj The object to display.
//! @return A reference to @a strm.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::DisplayInterface& obj)
{
    return obj.display(strm);
}
