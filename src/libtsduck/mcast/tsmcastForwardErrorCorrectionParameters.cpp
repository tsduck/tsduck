//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastForwardErrorCorrectionParameters.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::ForwardErrorCorrectionParameters::clear()
{
    scheme_identifier.clear();
    overhead_percentage = 0;
    endpoints.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::ForwardErrorCorrectionParameters::parseXML(const xml::Element* element, bool strict)
{
    if (element == nullptr) {
        return false;
    }

    bool ok = element->getTextChild(scheme_identifier, u"SchemeIdentifier", true, strict) &&
              element->getIntChild(overhead_percentage, u"OverheadPercentage", strict);

    for (auto& ep : element->children(u"EndpointAddress", &ok)) {
        ok = endpoints.emplace_back().parseXML(&ep, strict);
    }

    return ok;
}
