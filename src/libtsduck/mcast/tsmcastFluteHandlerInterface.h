//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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

namespace ts::mcast {

    class FluteFile;
    class FluteFDT;
    class NIPActualCarrierInformation;

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
        //! @param [in] file The received file.
        //!
        virtual void handleFluteFile(const FluteFile& file);

        //!
        //! This hook is invoked when a new File Delivery Table (FDT) is available.
        //! The default implementation does nothing.
        //! @param [in] fdt The received FDT.
        //!
        virtual void handleFluteFDT(const FluteFDT& fdt);

        //!
        //! This hook is invoked when a NIPActualCarrierInformation is found in a LCT header.
        //! The default implementation does nothing.
        //! @param [in] naci The received NIPActualCarrierInformation.
        //!
        virtual void handleFluteNACI(const NIPActualCarrierInformation& naci);
    };
}
