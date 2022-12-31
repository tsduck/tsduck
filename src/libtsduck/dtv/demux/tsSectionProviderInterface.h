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
//!  Abstract interface for classes which provide MPEG sections into
//!  a Packetizer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsTablesPtr.h"

namespace ts {
    //!
    //! Abstract interface for classes which provide MPEG sections into a Packetizer.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which provide
    //! MPEG sections into a Packetizer.
    //!
    class TSDUCKDLL SectionProviderInterface
    {
    public:
        //!
        //! This hook is invoked when a new section is required.
        //! @param [in] counter The section counter is an information on the progression
        //! (zero the first time the hook is invoked from the packetizer).
        //! @param [out] section A smart pointer to the next section to packetize.
        //! If a null pointer is provided, no section is available.
        //!
        virtual void provideSection(SectionCounter counter, SectionPtr& section) = 0;

        //!
        //! Shall we perform section stuffing.
        //! @return True if stuffing to the next transport packet boundary shall be
        //! performed before the next section. Unless explicitly allowed to, a Packetizer never
        //! splits a section header between two packets. This is not required by the MPEG standard
        //! but some STB are known to have problems with that.
        //!
        virtual bool doStuffing() = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~SectionProviderInterface();
    };
}
