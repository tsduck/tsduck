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
        const xml::Element* abn = nullptr;

        // Decode fixed elements.
        _valid = root->getISODateTimeChild(version_update, u"VersionUpdate", strict) &&
                 root->getTextChild(nif_type, u"NIFType", true, strict) &&
                 ((abn = root->findFirstChild(u"ActualBroadcastNetwork", strict)) != nullptr || !strict) &&
                 (abn == nullptr || actual.parseXML(abn, strict));

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

    streams.clear();
    for (auto& e : element->children(u"NIPStream", &ok, strict ? 1 : 0)) {
        streams.emplace_back();
        auto& st(streams.back());
        ok = e.getTextChild(st.link_layer_format, u"LinkLayerFormat", true, strict) &&
             e.getTextChild(st.provider_name, u"NIPStreamProviderName", true, strict) &&
             e.getIntChild(st.carrier_id, u"NIPCarrierID", strict) &&
             e.getIntChild(st.link_id, u"NIPLinkID", strict) &&
             e.getIntChild(st.service_id, u"NIPServiceID", strict);
    }

    return ok;
}
