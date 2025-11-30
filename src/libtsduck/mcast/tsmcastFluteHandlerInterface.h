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
    class UString;
}

namespace ts::mcast {

    class FluteDemux;
    class FluteFile;
    class FluteFDT;
    class FluteSessionId;
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

        //!
        //! This hook is invoked when a NIPActualCarrierInformation is found in a LCT header.
        //! The default implementation does nothing.
        //! @param [in,out] demux A reference to the FLUTE demux.
        //! @param [in] naci The received NIPActualCarrierInformation.
        //!
        virtual void handleFluteNACI(FluteDemux& demux, const NIPActualCarrierInformation& naci);

        //!
        //! This hook is invoked by FluteDemux::getFilesStatus() for each file.
        //! The default implementation does nothing.
        //! @param [in,out] demux A reference to the FLUTE demux.
        //! @param [in] session Session identification.
        //! @param [in] name File name. May be empty if partially transfered and not referenced yet in FDT.
        //! @param [in] type File type. May be empty as well.
        //! @param [in] toi Transport object identifier.
        //! @param [in] total_length Total announced file size in bytes.
        //! @param [in] received_length Number of received bytes so far.
        //!
        virtual void handleFluteStatus(FluteDemux& demux,
                                       const FluteSessionId& session,
                                       const UString& name,
                                       const UString& type,
                                       uint64_t toi,
                                       uint64_t total_length,
                                       uint64_t received_length);
    };
}
