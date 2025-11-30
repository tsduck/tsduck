//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMulticastGatewayConfiguration.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::MulticastGatewayConfiguration::MulticastGatewayConfiguration(Report& report, const FluteFile& file) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"MulticastGatewayConfiguration", true)) {
        const xml::Element* root = doc.rootElement();

        // Decode all MulticastGatewayConfigurationTransportSession elements.
        for (const xml::Element* e = root->findFirstChild(u"MulticastGatewayConfigurationTransportSession", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            transport_sessions.emplace_back();
            _valid = transport_sessions.back().parseXML(e);
        }

        // Decode all MulticastSession elements.
        for (const xml::Element* e = root->findFirstChild(u"MulticastSession", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            multicast_sessions.emplace_back();
            _valid = multicast_sessions.back().parseXML(e);
        }

        // Other elements of the <MulticastGatewayConfiguration> are not parsed (so far).
    }
}

ts::MulticastGatewayConfiguration::~MulticastGatewayConfiguration()
{
}
