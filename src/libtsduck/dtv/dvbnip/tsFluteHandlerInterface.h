//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  FLUTE demux handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class FluteDemux;
    class FluteFile;
    class FluteFDT;

    //!
    //! FLUTE demux handler interface.
    //! @ingroup libtsduck mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified about received files using a FluteDemux.
    //!
    class TSDUCKDLL FluteHandlerInterface
    {
        TS_INTERFACE(FluteHandlerInterface);
    public:
        //!
        //! This hook is invoked when a new file is available.
        //! The default implementation does nothing.
        //! @param [in,out] demux A reference to the FLUTE demux.
        //! @param [in] file The received file.
        //!
        virtual void handleFluteFile(FluteDemux& demux, const FluteFile& file);

        //!
        //! This hook is invoked when a new File Delivery Table (FDT) is available.
        //! The default implementation does nothing.
        //! @param [in,out] demux A reference to the FLUTE demux.
        //! @param [in] fdt The received FDT.
        //!
        virtual void handleFluteFDT(FluteDemux& demux, const FluteFDT& fdt);
    };
}
