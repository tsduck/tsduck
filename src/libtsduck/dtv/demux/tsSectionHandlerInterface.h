//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface to receive MPEG Section from a SectionDemux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSection.h"

namespace ts {

    class SectionDemux;

    //!
    //! Abstract interface to receive MPEG Section from a SectionDemux.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of individual sections using a SectionDemux.
    //!
    class TSDUCKDLL SectionHandlerInterface
    {
        TS_INTERFACE(SectionHandlerInterface);
    public:
        //!
        //! This hook is invoked when a complete section is available.
        //! @param [in,out] demux The demux which sends the section.
        //! @param [in] section The new section from the demux.
        //!
        virtual void handleSection(SectionDemux& demux, const Section& section) = 0;
    };
}
