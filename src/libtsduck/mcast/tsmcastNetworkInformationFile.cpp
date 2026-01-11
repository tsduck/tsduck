//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNetworkInformationFile.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::NetworkInformationFile::NetworkInformationFile(Report& report, const FluteFile& file, bool strict) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"NetworkInformationFile", true)) {
        const xml::Element* root = doc.rootElement();

        // Decode fixed elements.
        _valid = root->getISODateTimeChild(version_update, u"VersionUpdate", strict) &&
                 root->getTextChild(nif_type, u"NIFType", true, strict);

        // Decode exactly one ActualBroadcastNetwork element.
        for (auto& e : root->children(u"ActualBroadcastNetwork", &_valid, strict ? 1 : 0, 1)) {
            _valid = actual.parseXML(&e, strict);
        }

        // Decode all OtherBroadcastNetwork elements.
        for (auto& e : root->children(u"OtherBroadcastNetwork", &_valid)) {
            others.emplace_back();
            _valid = others.back().parseXML(&e, strict);
        }
    }
}

ts::mcast::NetworkInformationFile::~NetworkInformationFile()
{
}


//----------------------------------------------------------------------------
// Reinitialize a BroadcastNetwork from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::NetworkInformationFile::BroadcastNetwork::parseXML(const xml::Element* element, bool strict)
{
    bool ok = element != nullptr &&
              element->getTextChild(network_type, u"NetworkType", true, strict) &&
              element->getTextChild(network_name, u"NetworkName", true, strict) &&
              element->getTextChild(provider_name, u"NIPNetworkProviderName", true, strict) &&
              element->getIntChild(nip_network_id, u"NIPNetworkID", strict, 0, 1, 65280);

    // At most one <SatellitePosition>.
    satellite_position.reset();
    for (auto& e : element->children(u"SatellitePosition", &ok, 0, 1)) {
        emplace(satellite_position);
        ok = e.getFloatChild(satellite_position->orbital_position, u"OrbitalPosition", strict, 0.0, 0.0, 180.0) &&
             e.getTextChild(satellite_position->west_east, u"West_East_flag", true, strict);
    }

    // One or more <NIPStream>.
    streams.clear();
    for (auto& e : element->children(u"NIPStream", &ok, strict ? 1 : 0)) {
        auto& st(streams.emplace_back());
        ok = e.getTextChild(st.link_layer_format, u"LinkLayerFormat", true, strict) &&
             e.getTextChild(st.provider_name, u"NIPStreamProviderName", true, strict) &&
             e.getIntChild(st.carrier_id, u"NIPCarrierID", strict) &&
             e.getIntChild(st.link_id, u"NIPLinkID", strict) &&
             e.getIntChild(st.service_id, u"NIPServiceID", strict);

        // At most one <BootstrapStream>.
        for (auto& bs : e.children(u"BootstrapStream", &ok, 0, 1)) {
            emplace(st.bootstrap_stream);
            ok = bs.getTextChild(st.bootstrap_stream->bootstrap_type, u"BootstrapType", true, strict) &&
                 bs.getTextChild(st.bootstrap_stream->status, u"Status", true, strict);
        }

        // Exactly one of <DVBS2_NIPDeliveryParameters>, <DVBS2X_NIPDeliveryParameters>, <DVBT2_NIPDeliveryParameters>, <OtherDeliveryParameters>.
        for (auto& dp : e.children(u"DVBS2_NIPDeliveryParameters", &ok, 0, 1)) {
            emplace(st.dvbs2);
            ok = dp.getIntChild(st.dvbs2->frequency, u"Frequency", strict) &&
                 dp.getIntChild(st.dvbs2->symbol_rate, u"SymbolRate", strict) &&
                 dp.getIntChild(st.dvbs2->scrambling_sequence_index, u"scrambling_sequence_index", false, 0, 0, 0x3FFFF) &&
                 dp.getIntChild(st.dvbs2->input_stream_identifier, u"input_stream_identifier", strict) &&
                 dp.getTextChild(st.dvbs2->polarization, u"Polarization", true, strict) &&
                 dp.getTextChild(st.dvbs2->modulation_type, u"Modulation_Type", true, strict) &&
                 dp.getTextChild(st.dvbs2->roll_off, u"Roll_off", true, strict) &&
                 dp.getTextChild(st.dvbs2->fec, u"FEC", true, strict);
        }
        for (auto& dp : e.children(u"DVBS2X_NIPDeliveryParameters", &ok, 0, 1)) {
            emplace(st.dvbs2x);
            ok = dp.getTextChild(st.dvbs2x->receiver_profiles, u"receiver_profiles", true, strict) &&
                 dp.getTextChild(st.dvbs2x->s2x_mode, u"S2X_mode", true, strict) &&
                 dp.getIntChild(st.dvbs2x->frequency, u"Frequency", strict) &&
                 dp.getIntChild(st.dvbs2x->symbol_rate, u"SymbolRate", strict) &&
                 dp.getIntChild(st.dvbs2x->scrambling_sequence_index, u"scrambling_sequence_index", false, 0, 0, 0x3FFFF) &&
                 dp.getIntChild(st.dvbs2x->input_stream_identifier, u"input_stream_identifier", strict) &&
                 dp.getTextChild(st.dvbs2x->polarization, u"Polarization", true, strict) &&
                 dp.getTextChild(st.dvbs2x->roll_off, u"Roll_off", true, strict);

        }
        for (auto& dp : e.children(u"DVBT2_NIPDeliveryParameters", &ok, 0, 1)) {
            emplace(st.dvbt2);
            ok = dp.getIntChild(st.dvbt2->plp_id, u"plp_id", strict) &&
                 dp.getIntChild(st.dvbt2->t2_system_id, u"T2_system_id", strict);
            for (auto& xt2 : e.children(u"long_T2_system_delivery_descriptor", &ok, 0, 1)) {
                emplace(st.dvbt2->t2_desc);
                auto& t2(st.dvbt2->t2_desc.value());
                ok = xt2.getTextChild(t2.siso_miso, u"SISO_MISO", true, strict) &&
                     xt2.getTextChild(t2.bandwidth, u"bandwidth", true, strict) &&
                     xt2.getTextChild(t2.guard_interval, u"guard_interval", true, strict) &&
                     xt2.getTextChild(t2.transmission_type, u"transmission_type", true, strict) &&
                     xt2.getBoolChild(t2.other_frequency, u"other_frequency_flag", strict) &&
                     xt2.getBoolChild(t2.tfs, u"tfs_flag", strict) &&
                     xt2.getIntChild(t2.cell_id, u"cell_id", strict) &&
                     xt2.getFloatChild(t2.centre_frequency, u"centre_frequency", strict) &&
                     xt2.getIntChild(t2.cell_id_extension, u"cell_id_extension", false) &&
                     xt2.getFloatChild(t2.transposer_frequency, u"transposer_frequency", false);
            }
        }
    }

    return ok;
}
