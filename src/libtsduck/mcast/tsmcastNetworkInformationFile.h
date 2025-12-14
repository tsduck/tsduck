//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a NetworkInformationFile (DVB-NIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteFile.h"
#include "tsReport.h"
#include "tsTime.h"

namespace ts::mcast {
    //!
    //! Representation of a NetworkInformationFile (DVB-NIP).
    //! Caution: This implementation is partial. Some part of the XML document are not deserialized.
    //! @see ETSI TS 103 876, section 8.4.2.2
    //! @see ServiceInformationFile
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NetworkInformationFile : public FluteFile
    {
        TS_RULE_OF_FIVE(NetworkInformationFile, override);
    public:
        //!
        //! Default constructor.
        //!
        NetworkInformationFile() = default;

        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] file Received file from FLUTE demux.
        //!
        NetworkInformationFile(Report& report, const FluteFile& file);

        //!
        //! Definition of a \<NIPStream>.
        //!
        class TSDUCKDLL NIPStream
        {
        public:
            NIPStream() = default;          //!< Constructor.
            UString  link_layer_format {};  //!< Element \<LinkLayerFormat>
            UString  provider_name {};      //!< Element \<NIPStreamProviderName>
            uint16_t carrier_id = 0;        //!< Element \<NIPCarrierID>
            uint16_t link_id = 0;           //!< Element \<NIPLinkID>
            uint16_t service_id = 0;        //!< Element \<NIPServiceID>
        };

        //!
        //! Definition of a \<ActualBroadcastNetwork> or \<OtherBroadcastNetwork>.
        //!
        class TSDUCKDLL BroadcastNetwork
        {
        public:
            BroadcastNetwork() = default;     //!< Constructor.
            UString  network_type {};         //!< Element \<NetworkType>
            UString  network_name {};         //!< Element \<NetworkName>
            UString  provider_name {};        //!< Element \<NIPNetworkProviderName>
            uint16_t nip_network_id = 0;      //!< Element \<NIPNetworkID>
            std::list<NIPStream> streams {};  //!< Elements \<NIPStream>

            //!
            //! Reinitialize the structure from a XML element.
            //! @param [in] element Root XML element to analyze.
            //! @return True on success, false on error.
            //!
            bool parseXML(const xml::Element* element);
        };

        // NetworkInformationFile public fields.
        Time                        version_update {};  //!< Element \<VersionUpdate>
        UString                     nif_type {};        //!< Element \<NIFType>
        BroadcastNetwork            actual {};          //!< Element \<ActualBroadcastNetwork>
        std::list<BroadcastNetwork> others {};          //!< Elements \<OtherBroadcastNetwork>
    };
}
