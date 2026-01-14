//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastBaseMulticastTransportSession.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::BaseMulticastTransportSession::clear()
{
    service_class.clear();
    transport_security.clear();
    bitrate_average = bitrate_maximum = 0;
    repair_base_url.clear();
    repair_obj_base_uri.clear();
    repair_recv_timeout = repair_fixed_backoff = repair_rand_backoff = cn::milliseconds::zero();
    protocol.clear();
    endpoints.clear();
    fec.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::BaseMulticastTransportSession::parseXML(const xml::Element* element, bool strict)
{
    clear();
    if (element == nullptr) {
        return false;
    }

    bool ok = element->getAttribute(service_class, u"serviceClass") &&
              element->getAttribute(transport_security, u"transportSecurity", false, u"none") &&
              protocol.parseXML(element, strict);

    for (auto& e : element->children(u"EndpointAddress", &ok, strict ? 1 : 0)) {
        ok = endpoints.emplace_back().parseXML(&e, strict);
    }
    for (auto& e : element->children(u"BitRate", &ok, strict ? 1 : 0, 1)) {
        ok = e.getIntAttribute(bitrate_average, u"average", false) &&
             e.getIntAttribute(bitrate_maximum, u"maximum", strict);
    }
    for (auto& e : element->children(u"ForwardErrorCorrectionParameters", &ok)) {
        ok = fec.emplace_back().parseXML(&e, strict);
    }
    for (auto& e : element->children(u"UnicastRepairParameters", &ok, 0, 1)) {
        ok = e.getAttribute(repair_obj_base_uri, u"transportObjectBaseURI", false) &&
             e.getChronoAttribute(repair_recv_timeout, u"transportObjectReceptionTimeout", strict) &&
             e.getChronoAttribute(repair_fixed_backoff, u"fixedBackOffPeriod", false) &&
             e.getChronoAttribute(repair_rand_backoff, u"randomBackOffPeriod", false);
        for (auto& xbu : e.children(u"BaseURL", &ok)) {
            auto& bu(repair_base_url.emplace_back());
            ok = xbu.getText(bu.uri, strict) &&
                 xbu.getIntAttribute(bu.relative_weight, u"relativeWeight", false, 1);
        }
    }

    return ok;
}
