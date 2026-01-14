//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastReportingLocator.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::ReportingLocator::clear()
{
    uri.clear();
    proportion = 1.0;
    period = random_delay = cn::milliseconds::zero();
    report_session_running_events = false;
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::ReportingLocator::parseXML(const xml::Element* element, bool strict)
{
    return element != nullptr &&
           element->getText(uri, true) &&
           element->getFloatAttribute(proportion, u"proportion", false, 1.0) &&
           element->getISODurationAttribute(period, u"period", strict, strict) &&
           element->getChronoAttribute(random_delay, u"randomDelay", strict) &&
           element->getBoolAttribute(report_session_running_events, u"reportSessionRunningEvents", false);
}
