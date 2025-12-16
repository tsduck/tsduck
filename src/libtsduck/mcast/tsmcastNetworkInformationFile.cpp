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

ts::mcast::NetworkInformationFile::NetworkInformationFile(Report& report, const FluteFile& file, bool strict) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"NetworkInformationFile", true)) {
        const xml::Element* root = doc.rootElement();
        const xml::Element* e = nullptr;

        // Decode fixed elements.
        UString time;
        _valid = root->getTextChild(time, u"VersionUpdate", true, strict) &&
                 root->getTextChild(nif_type, u"NIFType", true, strict) &&
                 ((e = root->findFirstChild(u"ActualBroadcastNetwork", !strict)) != nullptr || !strict) &&
                 (e == nullptr || actual.parseXML(e, strict));
        if (_valid) {
            version_update.fromISO(time);
        }

        // Decode all OtherBroadcastNetwork elements.
        for (e = root->findFirstChild(u"OtherBroadcastNetwork", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            others.emplace_back();
            _valid = others.back().parseXML(e, strict);
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

    streams.clear();
    for (const xml::Element* e = element->findFirstChild(u"NIPStream", !strict); ok && e != nullptr; e = e->findNextSibling(true)) {
        streams.emplace_back();
        auto& st(streams.back());
        ok = e->getTextChild(st.link_layer_format, u"LinkLayerFormat", true, strict) &&
             e->getTextChild(st.provider_name, u"NIPStreamProviderName", true, strict) &&
             e->getIntChild(st.carrier_id, u"NIPCarrierID", strict) &&
             e->getIntChild(st.link_id, u"NIPLinkID", strict) &&
             e->getIntChild(st.service_id, u"NIPServiceID", strict);
    }

    return ok;
}
