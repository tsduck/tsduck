//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
