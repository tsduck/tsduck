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
#include "tsFluteFile.h"
#include "tsDisplayInterface.h"
#include "tsReport.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a ServiceInformationFile (DVB-NIP).
    //! @see ETSI TS 103 876, section 8.4.3.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ServiceInformationFile : public FluteFile, public DisplayInterface
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

        // Inherited methods.
        virtual std::ostream& display(std::ostream& stream = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

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
        class TSDUCKDLL BroadcastMediaStream
        {
        public:
            BroadcastMediaStream() = default;            //!< Constructor.
            uint16_t    nip_network_id = 0;              //!< Element \<NIPNetworkID>
            uint16_t    nip_carrier_id = 0;              //!< Element \<NIPCarrierID>
            uint16_t    nip_link_id = 0;                 //!< Element \<NIPLinkID>
            uint16_t    nip_service_id = 0;              //!< Element \<NIPServiceID>
            UStringList uri {};                          //!< Elements \<URI>
            std::list<InteractiveApplications> apps {};  //!< Elements \<InteractiveApplications>
        };

        // ServiceInformationFile public fields.
        Time    version_update {};                   //!< Element \<VersionUpdate>
        UString provider_name {};                    //!< Element \<NIPNetworkProviderName>
        std::list<BroadcastMediaStream> streams {};  //!< Elements \<BroadcastMediaStream>
    };
}
