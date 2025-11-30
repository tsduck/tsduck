//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a MulticastGatewayConfiguration (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsFluteFile.h"
#include "tsMulticastGatewayConfigurationTransportSession.h"
#include "tsMulticastSession.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Representation of a MulticastGatewayConfiguration (Multicast ABR).
    //! Caution: This implementation is partial. Some part of the XML document are not deserialized.
    //! @see ETSI TS 103 769, section 10.2.1.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL MulticastGatewayConfiguration : public FluteFile
    {
        TS_RULE_OF_FIVE(MulticastGatewayConfiguration, override);
    public:
        //!
        //! Default constructor.
        //!
        MulticastGatewayConfiguration() = default;

        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] file Received file from FLUTE demux.
        //!
        MulticastGatewayConfiguration(Report& report, const FluteFile& file);

        //!
        //! List of MulticastGatewayConfigurationTransportSession.
        //!
        std::list<MulticastGatewayConfigurationTransportSession> transport_sessions {};

        //!
        //! List of MulticastSession.
        //!
        std::list<MulticastSession> multicast_sessions {};
    };
}
