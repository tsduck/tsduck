//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a ServiceInformationFile (DVB-NIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteFile.h"
#include "tsmcastNIPStreamId.h"
#include "tsReport.h"
#include "tsTime.h"

namespace ts::mcast {
    //!
    //! Representation of a ServiceInformationFile (DVB-NIP).
    //! @see ETSI TS 103 876, section 8.4.3.2
    //! @see NetworkInformationFile
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ServiceInformationFile : public FluteFile
    {
        TS_RULE_OF_FIVE(ServiceInformationFile, override);
    public:
        //!
        //! Default constructor.
        //!
        ServiceInformationFile() = default;

        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] file Received file from FLUTE demux.
        //!
        ServiceInformationFile(Report& report, const FluteFile& file);

        //!
        //! Definition of a \<InteractiveApplications> element in a \<BroadcastMedia>.
        //!
        class TSDUCKDLL InteractiveApplications
        {
        public:
            InteractiveApplications() = default;  //!< Constructor.
            int32_t id = 0;                       //!< Element \<ApplicationID>.
            UString type {};                      //!< Element \<ApplicationType>.
            UString uri {};                       //!< Element \<ApplicationURI>.
        };

        //!
        //! Definition of a \<BroadcastMediaStream> element.
        //!
        //! There is one BroadcastMediaStream per NIP Stream.
        //! Each URI may point to:
        //! - A service list file (.xml).
        //! - A HLS play list (.m3u8) for a service.
        //! - A MPEG-DASH manifest (.mpd) for a service.
        //!
        class TSDUCKDLL BroadcastMediaStream
        {
        public:
            BroadcastMediaStream() = default;            //!< Constructor.
            NIPStreamId stream_id {};                    //!< NIP stream id.
            UStringList uri {};                          //!< Elements \<URI>
            std::list<InteractiveApplications> apps {};  //!< Elements \<InteractiveApplications>
        };

        // ServiceInformationFile public fields.
        Time    version_update {};                   //!< Element \<VersionUpdate>
        UString provider_name {};                    //!< Element \<NIPNetworkProviderName>
        std::list<BroadcastMediaStream> streams {};  //!< Elements \<BroadcastMediaStream>
    };
}
