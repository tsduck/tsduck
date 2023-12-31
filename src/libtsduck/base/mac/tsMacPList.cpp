//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMacPList.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::MacPList::MacPList(const ts::UString& fileName, Report& report) : SuperClass()
{
    if (!fileName.empty()) {
        load(fileName, report);
    }
}


//----------------------------------------------------------------------------
// Reload from a MacOS XML PList file.
//----------------------------------------------------------------------------

bool ts::MacPList::load(const ts::UString& fileName, Report& report)
{
    // Load the XML file.
    xml::Document doc(report);
    if (!doc.load(fileName, false)) {
        return false;
    }

    // Get the root of the document, expected as <plist>
    const xml::Element* root = doc.rootElement();

    // Get the <dict> element inside <plist>
    const xml::Element* dict = root == nullptr ? nullptr : root->findFirstChild(u"dict");
    if (dict == nullptr) {
        return false;
    }

    // Load all pairs of <key>Name</key> <string>Value</string>
    for (const xml::Element* child = dict->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (child->name().similar(u"key")) {
            const xml::Element* next = child->nextSiblingElement();
            if (next != nullptr && next->name().similar(u"string")) {
                // Found a <key>Name</key> <string>Value</string>
                UString name;
                UString value;
                if (child->getText(name) && next->getText(value) && !name.empty()) {
                    insert(std::make_pair(name, value));
                }
            }
        }
    }

    return true;
}
