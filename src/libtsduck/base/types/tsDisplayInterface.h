//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(DisplayInterface);
    public:
        //!
        //! Display the content of this object to a stream.
        //! @param [in,out] stream The stream where to print the content. Standard output by default.
        //! @param [in] margin The prefix string on each line, empty by default.
        //! @param [in] level Severity level (for instance, Severity::Info or Severity::Debug may display more information).
        //! @return A reference to @a stream.
        //!
        virtual std::ostream& display(std::ostream& stream = std::cout, const UString& margin = UString(), int level = Severity::Info) const = 0;
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
