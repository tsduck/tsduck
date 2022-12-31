//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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

ts::TimeConfigurationFile::TimeConfigurationFile() :
    initial_seconds(0),
    leap_seconds()
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
