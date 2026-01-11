//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //!
        NetworkInformationFile(Report& report, const FluteFile& file, bool strict);

        //!
        //! Definition of a \<SatellitePosition>.
        //!
        class TSDUCKDLL SatellitePosition
        {
        public:
            SatellitePosition() = default;   //!< Constructor.
            double  orbital_position = 0.0;  //!< Element \<OrbitalPosition>
            UString west_east {};            //!< Element \<West_East_flag>
        };

        //!
        //! Definition of a \<BootstrapStream>.
        //!
        class TSDUCKDLL BootstrapStream
        {
        public:
            BootstrapStream() = default;  //!< Constructor.
            UString bootstrap_type {};    //!< Element \<BootstrapType>.
            UString status {};            //!< Element \<Status>.
        };

        //!
        //! Definition of a \<DVBS2_NIPDeliveryParameters>.
        //!
        class TSDUCKDLL DVBS2_NIPDeliveryParameters
        {
        public:
            DVBS2_NIPDeliveryParameters() = default;  //!< Constructor.
            uint64_t frequency = 0;                   //!< Element \<Frequency> in units of 10 kHz.
            uint64_t symbol_rate = 0;                 //!< Element \<SymbolRate> in units of 1 k sym/s.
            UString  polarization {};                 //!< Element \<Polarization>.
            UString  modulation_type {};              //!< Element \<Modulation_Type>.
            UString  roll_off {};                     //!< Element \<Roll_off>.
            UString  fec {};                          //!< Element \<FEC>.
            uint32_t scrambling_sequence_index = 0;   //!< Element \<scrambling_sequence_index>, 0 to 262143 (0x3FFFF).
            uint8_t  input_stream_identifier = 0;     //!< Element \<input_stream_identifier>.
        };

        //!
        //! Definition of a \<DVBS2X_NIPDeliveryParameters>.
        //!
        class TSDUCKDLL DVBS2X_NIPDeliveryParameters
        {
        public:
            DVBS2X_NIPDeliveryParameters() = default; //!< Constructor.
            UString  receiver_profiles {};            //!< Element \<receiver_profiles>.
            UString  s2x_mode {};                     //!< Element \<S2X_mode>.
            uint64_t frequency = 0;                   //!< Element \<Frequency> in units of 10 kHz.
            uint64_t symbol_rate = 0;                 //!< Element \<SymbolRate> in units of 1 k sym/s.
            UString  polarization {};                 //!< Element \<Polarization>.
            UString  roll_off {};                     //!< Element \<Roll_off>.
            uint32_t scrambling_sequence_index = 0;   //!< Element \<scrambling_sequence_index>, 0 to 262143 (0x3FFFF).
            uint8_t  input_stream_identifier = 0;     //!< Element \<input_stream_identifier>.
        };

        //!
        //! Definition of a \<long_T2_system_delivery_descriptor>.
        //!
        class TSDUCKDLL T2_descriptor
        {
        public:
            T2_descriptor() = default;          //!< Constructor.
            UString  siso_miso {};              //!< Element \<SISO_MISO>.
            UString  bandwidth {};              //!< Element \<bandwidth>.
            UString  guard_interval {};         //!< Element \<guard_interval>.
            UString  transmission_type {};      //!< Element \<transmission_type>.
            bool     other_frequency = false;   //!< Element \<other_frequency_flag>.
            bool     tfs = false;               //!< Element \<tfs_flag>.
            uint16_t cell_id = 0;               //!< Element \<cell_id>.
            double   centre_frequency = 0;      //!< Element \<centre_frequency>.
            uint16_t cell_id_extension = 0;     //!< Element \<cell_id_extension>, optional.
            double   transposer_frequency = 0;  //!< Element \<transposer_frequency>, optional.
        };

        //!
        //! Definition of a \<DVBT2_NIPDeliveryParameters>.
        //!
        class TSDUCKDLL DVBT2_NIPDeliveryParameters
        {
        public:
            DVBT2_NIPDeliveryParameters() = default;  //!< Constructor.
            uint8_t  plp_id = 0;                      //!< Element \<plp_id>.
            uint16_t t2_system_id = 0;                //!< Element \<T2_system_id>.
            std::optional<T2_descriptor> t2_desc {};  //!< Element \<long_T2_system_delivery_descriptor>.
        };

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
            std::optional<BootstrapStream> bootstrap_stream {};     //!< Element \<BootstrapStream>.
            std::optional<DVBS2_NIPDeliveryParameters> dvbs2 {};    //!< Element \<DVBS2_NIPDeliveryParameters>.
            std::optional<DVBS2X_NIPDeliveryParameters> dvbs2x {};  //!< Element \<DVBS2X_NIPDeliveryParameters>.
            std::optional<DVBT2_NIPDeliveryParameters> dvbt2 {};    //!< Element \<DVBT2_NIPDeliveryParameters>.
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
            std::optional<SatellitePosition> satellite_position {};  //!< Elements \<SatellitePosition>

            //!
            //! Reinitialize the structure from a XML element.
            //! @param [in] element Root XML element to analyze.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //! @return True on success, false on error.
            //!
            bool parseXML(const xml::Element* element, bool strict);
        };

        // NetworkInformationFile public fields.
        Time                        version_update {};  //!< Element \<VersionUpdate>
        UString                     nif_type {};        //!< Element \<NIFType>
        BroadcastNetwork            actual {};          //!< Element \<ActualBroadcastNetwork>
        std::list<BroadcastNetwork> others {};          //!< Elements \<OtherBroadcastNetwork>
    };
}
