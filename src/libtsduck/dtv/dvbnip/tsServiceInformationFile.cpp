//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceInformationFile.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ServiceInformationFile::ServiceInformationFile(Report& report, const FluteFile& file) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"ServiceInformationFile", true)) {
        const xml::Element* root = doc.rootElement();

        // Decode fixed elements.
        UString time;
        _valid = root->getTextChild(time, u"VersionUpdate", true, true) && root->getTextChild(provider_name, u"NIPNetworkProviderName", true, true);
        if (_valid) {
            version_update.fromISO(time);
        }

        // Decode all BroadcastMediaStream elements.
        for (const xml::Element* e = root->findFirstChild(u"BroadcastMediaStream", false); _valid && e != nullptr; e = e->findNextSibling(true)) {
            streams.emplace_back();
            auto& st(streams.back());
            const xml::Element* bmedia = nullptr;
            _valid = e->getIntChild(st.nip_network_id, u"NIPNetworkID", true, 0, 1, 65280) &&
                     e->getIntChild(st.nip_carrier_id, u"NIPCarrierID", true) &&
                     e->getIntChild(st.nip_link_id, u"NIPLinkID", true) &&
                     e->getIntChild(st.nip_service_id, u"NIPServiceID", true) &&
                     (bmedia = e->findFirstChild(u"BroadcastMedia")) != nullptr;
            if (_valid) {
                for (const xml::Element* e1 = bmedia->findFirstChild(u"URI", true); _valid && e1 != nullptr; e1 = e1->findNextSibling(true)) {
                    st.uri.emplace_back();
                    _valid = e1->getText(st.uri.back(), true);
                }
                for (const xml::Element* e1 = bmedia->findFirstChild(u"InteractiveApplications", true); _valid && e1 != nullptr; e1 = e1->findNextSibling(true)) {
                    st.apps.emplace_back();
                    _valid = e1->getTextChild(st.apps.back().type, u"ApplicationType", true, true) &&
                             e1->getTextChild(st.apps.back().uri, u"ApplicationURI", true, true) &&
                             e1->getIntChild(st.apps.back().id, u"ApplicationID", true);
                }
            }
        }
    }
}

ts::ServiceInformationFile::~ServiceInformationFile()
{
}


//----------------------------------------------------------------------------
// Display the content of this structure.
//----------------------------------------------------------------------------

std::ostream& ts::ServiceInformationFile::display(std::ostream& out, const UString& margin, int level) const
{
    out << margin << "ServiceInformationFile: " << streams.size() << " streams" << std::endl
        << margin << "  Version update: " << version_update << ", provider: \"" << provider_name << "\"" << std::endl;
    int count = 0;
    for (const auto& st : streams) {
        out << margin << "- BroadcastMediaStream " << ++count << ":" << std::endl
            << margin << "  NIP network: " << st.nip_network_id << ", carrier: " << st.nip_carrier_id
            << ", link: " << st.nip_link_id << ", service: " << st.nip_service_id << std::endl;
        for (const auto& uri : st.uri) {
            out << margin << "  URI: " << uri << std::endl;
        }
        for (const auto& app : st.apps) {
            out << margin << "  App: id: " << app.id << ", URI: " << app.uri << ", type: " << app.uri << std::endl;
        }
    }
    return out;
}
