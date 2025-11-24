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
#include "tsDisplayInterface.h"
#include "tsMulticastGatewayConfigurationTransportSession.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Representation of a MulticastGatewayConfiguration (Multicast ABR).
    //! @see ETSI TS 103 769, section 10.2.1.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL MulticastGatewayConfiguration : public FluteFile, public DisplayInterface
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

        // Inherited methods.
        virtual std::ostream& display(std::ostream& stream = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! List of MulticastGatewayConfigurationTransportSession.
        //! This is currently the only structures which are extracted from XML.
        //!
        std::list<MulticastGatewayConfigurationTransportSession> sessions {};
    };
}
