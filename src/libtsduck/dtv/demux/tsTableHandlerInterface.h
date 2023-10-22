//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Table handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class BinaryTable;
    class SectionDemux;

    //!
    //! Table handler interface.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of tables using a SectionDemux.
    //!
    class TSDUCKDLL TableHandlerInterface
    {
        TS_INTERFACE(TableHandlerInterface);
    public:
        //!
        //! This hook is invoked when a complete table is available.
        //! Tables with long sections are reported only when a new version is available.
        //! @param [in,out] demux A reference to the section demux.
        //! @param [in] table A reference to the demultiplexed table.
        //!
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) = 0;
    };
}
