//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface to be notified of Teletext messages using a Teletext demux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TeletextFrame;
    class TeletextDemux;

    //!
    //! Interface to be implemented by classes which need to be notified of Teletext messages using a Teletext demux.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TeletextHandlerInterface
    {
        TS_INTERFACE(TeletextHandlerInterface);
    public:
        //!
        //! This hook is invoked when a complete Teletext message is available.
        //! @param [in,out] demux The Teletext demux.
        //! @param [in] frame Teletext frame.
        //!
        virtual void handleTeletextMessage(TeletextDemux& demux, const TeletextFrame& frame) = 0;
    };
}
