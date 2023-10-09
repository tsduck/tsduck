//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsTimeConfigurationFile.h"
#include "tsxmlModelDocument.h"
#include "tsxmlElement.h"
#include "tsCerrReport.h"

// Singleton definition.
TS_DEFINE_SINGLETON(ts::TimeConfigurationFile);


//-----------------------------------------------------------------------------
// Constructor, load the configuration file.
//-----------------------------------------------------------------------------

ts::TimeConfigurationFile::TimeConfigurationFile()
{
    // Load the configuration XML file and model. Search them in TSDuck directory if the default file is used.
    xml::Document doc(CERR);
    xml::ModelDocument model(CERR);
    if (!doc.load(u"tsduck.time.xml", true) || !model.load(u"tsduck.time.model.xml", true) || !model.validate(doc)) {
        return;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();
    const xml::Element* xleap_root = root == nullptr ? nullptr : root->findFirstChild(u"leap_seconds");
    if (xleap_root == nullptr) {
        return;
    }

    // Get leap seconds configuration.
    xml::ElementVector xleap;
    xleap_root->getIntAttribute(initial_seconds, u"initial", true, 0);
    xleap_root->getChildren(xleap, u"leap");
    for (const auto& it : xleap) {
        LeapSecond ls;
        if (it->getDateTimeAttribute(ls.after, u"after", true) && it->getIntAttribute(ls.count, u"count", true)) {
            leap_seconds.push_back(ls);
        }
    }

    // Sort vector of leap seconds per date.
    std::sort(leap_seconds.begin(), leap_seconds.end());
}


//-----------------------------------------------------------------------------
// Get the number of leap seconds between two UTC dates.
//-----------------------------------------------------------------------------

ts::Second ts::TimeConfigurationFile::leapSeconds(const Time& start, const Time& end) const
{
    Second total = 0;
    if (!leap_seconds.empty() && start < end) {

        // Find index of first leap_seconds entry after start.
        size_t index = 0;
        while (index < leap_seconds.size() && leap_seconds[index].after < start) {
            index++;
        }

        // Add all leap seconds until we moved after end.
        while (index < leap_seconds.size() && leap_seconds[index].after < end) {
            total += leap_seconds[index++].count;
        }

        // If any date is before 1972 (first leap second), we cannot really know how many leap seconds there are.
        // If start and end surround the first leap second (1972), use initial leap seconds (10).
        // There should be another milestone: the TAI Epoch (1958). But since UNIX systems cannot represent
        // times before 1970, we just ignore.
        if (start < leap_seconds[0].after && end >= leap_seconds[0].after) {
            total += initial_seconds;
        }
    }
    return total;
}
