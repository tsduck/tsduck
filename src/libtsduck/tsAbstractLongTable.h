//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Abstract base class for MPEG PSI/SI tables with long sections
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI tables with long sections.
    //! @ingroup psi
    //!
    class TSDUCKDLL AbstractLongTable: public AbstractTable
    {
    public:
        // Common public members:
        uint8_t version;     //!< Table version number.
        bool    is_current;  //!< True if table is current, false if table is next.

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] version_ Table version number.
        //! @param [in] is_current_ True if table is current, false if table is next.
        //!
        AbstractLongTable(TID tid, const UChar* xml_name, uint8_t version_ = 0, bool is_current_ = true) :
            AbstractTable(tid, xml_name),
            version(version_),
            is_current(is_current_)
        {
        }
    };
}
