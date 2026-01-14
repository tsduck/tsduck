//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastGatewayConfiguration.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::GatewayConfiguration::GatewayConfiguration(Report& report, const FluteFile& file, bool strict) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"MulticastGatewayConfiguration", true)) {
        const xml::Element* root = doc.rootElement();

        _valid = root->getISODateTimeAttribute(validity_period, u"validityPeriod", false, strict) &&
                 root->getISODateTimeAttribute(valid_until, u"validUntil", false, strict);

        // Decode all GatewayConfigurationTransportSession elements.
        for (auto& e : root->children(u"MulticastGatewayConfigurationTransportSession", &_valid)) {
            _valid = transport_sessions.emplace_back().parseXML(&e, strict);
        }

        // Decode all MulticastSession elements.
        for (auto& e : root->children(u"MulticastSession", &_valid)) {
            _valid = multicast_sessions.emplace_back().parseXML(&e, strict);
        }

        // Decode at most one MulticastGatewaySessionReporting element.
        for (auto& e1 : root->children(u"MulticastGatewaySessionReporting", &_valid, 0, 1)) {
            for (auto& e2 : e1.children(u"ReportingLocator", &_valid, strict ? 1 : 0)) {
                _valid = reporting_locators.emplace_back().parseXML(&e2, strict);
            }
        }
    }
}

ts::mcast::GatewayConfiguration::~GatewayConfiguration()
{
}
