//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------

#include "tsMacPList.h"
#include "tsCerrReport.h"
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
