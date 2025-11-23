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
#include "tsReport.h"

namespace ts {
    //!
    //! Representation of a MulticastGatewayConfiguration (Multicast ABR).
    //! @see ETSI TS 103 769, section 10.2.1.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL MulticastGatewayConfiguration : public FluteFile
    {
    public:
        //!
        //! Default constructor.
        //!
        MulticastGatewayConfiguration() = default;

        //!
        //! Constructor.
        //! @param [in] report Where to report errors.
        //! @param [in] sid Session id.
        //! @param [in] toi Transport Object Identifier.
        //! @param [in] name File name or URN.
        //! @param [in] type File MIME type.
        //! @param [in] content File content.
        //!
        MulticastGatewayConfiguration(Report&               report,
                                      const FluteSessionId& sid,
                                      uint64_t              toi,
                                      const UString&        name,
                                      const UString&        type,
                                      const ByteBlockPtr&   content);

        //!
        //! List of MulticastGatewayConfigurationTransportSession.
        //! This is currently the only structures which are extracted from XML.
        //!
        std::list<MulticastGatewayConfigurationTransportSession> sessions {};
    };
}
