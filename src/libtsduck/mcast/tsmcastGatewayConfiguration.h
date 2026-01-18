//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a MulticastGatewayConfiguration (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteFile.h"
#include "tsmcastGatewayConfigurationTransportSession.h"
#include "tsmcastMulticastSession.h"
#include "tsmcastReportingLocator.h"
#include "tsISOTime.h"
#include "tsReport.h"

namespace ts::mcast {
    //!
    //! Representation of a MulticastGatewayConfiguration (Multicast ABR).
    //! @see ETSI TS 103 769, section 10.2.1.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL GatewayConfiguration : public FluteFile
    {
        TS_RULE_OF_FIVE(GatewayConfiguration, override);
    public:
        //!
        //! Default constructor.
        //!
        GatewayConfiguration() = default;

        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] file Received file from FLUTE demux.
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //!
        GatewayConfiguration(Report& report, const FluteFile& file, bool strict);

        // GatewayConfiguration public fields.
        uint32_t schema_version = 0;  //!< Attribute schemaVersion
        ISOTime  validity_period {};  //!< Attribute validityPeriod
        Time     valid_until {};      //!< Attribute validUntil (Time::Epoch if absent)
        std::list<GatewayConfigurationTransportSession> transport_sessions {};  //!< Elements \<GatewayConfigurationTransportSession>
        std::list<MulticastSession> multicast_sessions {};                      //!< Elements \<MulticastSession>
        std::list<ReportingLocator> reporting_locators {};                      //!< Elements \<ReportingLocator> in \<MulticastGatewaySessionReporting>
    };
}
