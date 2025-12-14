//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNetworkInformationFile.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::NetworkInformationFile::NetworkInformationFile(Report& report, const FluteFile& file) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"NetworkInformationFile", true)) {
        const xml::Element* root = doc.rootElement();
        const xml::Element* e = nullptr;

        // Decode fixed elements.
        UString time;
        _valid = root->getTextChild(time, u"VersionUpdate", true, true) &&
                 root->getTextChild(nif_type, u"NIFType", true, true) &&
                 (e = root->findFirstChild(u"ActualBroadcastNetwork", false)) != nullptr &&
                 actual.parseXML(e);
        if (_valid) {
            version_update.fromISO(time);
        }

        // Decode all OtherBroadcastNetwork elements.
        for (e = root->findFirstChild(u"OtherBroadcastNetwork", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            others.emplace_back();
            _valid = others.back().parseXML(e);
        }
    }
}

ts::mcast::NetworkInformationFile::~NetworkInformationFile()
{
}


//----------------------------------------------------------------------------
// Reinitialize a BroadcastNetwork from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::NetworkInformationFile::BroadcastNetwork::parseXML(const xml::Element* element)
{
    bool ok = element != nullptr &&
              element->getTextChild(network_type, u"NetworkType", true, true) &&
              element->getTextChild(network_name, u"NetworkName", true, true) &&
              element->getTextChild(provider_name, u"NIPNetworkProviderName", true, true) &&
              element->getIntChild(nip_network_id, u"NIPNetworkID", true, 0, 1, 65280);

    streams.clear();
    for (const xml::Element* e = element->findFirstChild(u"NIPStream", false); ok && e != nullptr; e = e->findNextSibling(true)) {
        streams.emplace_back();
        auto& st(streams.back());
        ok = e->getTextChild(st.link_layer_format, u"LinkLayerFormat", true, true) &&
             e->getTextChild(st.provider_name, u"NIPStreamProviderName", true, true) &&
             e->getIntChild(st.carrier_id, u"NIPCarrierID", true) &&
             e->getIntChild(st.link_id, u"NIPLinkID", true) &&
             e->getIntChild(st.service_id, u"NIPServiceID", true);
    }

    return ok;
}
