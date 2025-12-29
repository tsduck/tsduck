//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastServiceInformationFile.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::ServiceInformationFile::ServiceInformationFile(Report& report, const FluteFile& file, bool strict) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"ServiceInformationFile", true)) {
        const xml::Element* root = doc.rootElement();

        // Decode fixed elements.
        _valid = root->getISODateTimeChild(version_update, u"VersionUpdate", strict) &&
                 root->getTextChild(provider_name, u"NIPNetworkProviderName", true, strict);

        // Decode all BroadcastMediaStream elements.
        for (auto& e : root->children(u"BroadcastMediaStream", &_valid, strict ? 1 : 0)) {
            streams.emplace_back();
            auto& st(streams.back());
            const xml::Element* bmedia = nullptr;
            _valid = e.getIntChild(st.stream_id.network_id, u"NIPNetworkID", strict, 0, 1, 65280) &&
                     e.getIntChild(st.stream_id.carrier_id, u"NIPCarrierID", strict) &&
                     e.getIntChild(st.stream_id.link_id, u"NIPLinkID", strict) &&
                     e.getIntChild(st.stream_id.service_id, u"NIPServiceID", strict) &&
                     ((bmedia = e.findFirstChild(u"BroadcastMedia", strict)) != nullptr || !strict);
            if (_valid && bmedia != nullptr) {
                for (auto& e1 : bmedia->children(u"URI", &_valid)) {
                    st.uri.emplace_back();
                    _valid = e1.getText(st.uri.back(), true);
                }
                for (auto& e1 : bmedia->children(u"InteractiveApplications", &_valid)) {
                    st.apps.emplace_back();
                    _valid = e1.getTextChild(st.apps.back().type, u"ApplicationType", true, strict) &&
                             e1.getTextChild(st.apps.back().uri, u"ApplicationURI", true, strict) &&
                             e1.getIntChild(st.apps.back().id, u"ApplicationID", strict);
                }
            }
        }
    }
}

ts::mcast::ServiceInformationFile::~ServiceInformationFile()
{
}
