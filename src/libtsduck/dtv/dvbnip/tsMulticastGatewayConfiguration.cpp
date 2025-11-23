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
// Constructor.
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
            sessions.emplace_back();
            _valid = sessions.back().parseXML(e);
        }

        // Other elements of the <MulticastGatewayConfiguration> are not parsed (so far).
    }
}


//----------------------------------------------------------------------------
// Display the content of this structure.
//----------------------------------------------------------------------------

std::ostream& ts::MulticastGatewayConfiguration::display(std::ostream& out, const UString& margin, int level) const
{
    out << margin << "MulticastGatewayConfiguration: " << sessions.size() << " sessions" << std::endl;
    int count = 0;
    for (const auto& it : sessions) {
        out << margin << "- TransportSession " << ++count << ":" << std::endl;
        it.display(out, margin + u"  ");
    }
    return out;
}
