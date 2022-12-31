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

#include "tsxmlUnknown.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Unknown::Unknown(Report& report, size_t line) :
    Node(report, line)
{
}

ts::xml::Unknown::Unknown(Node* parent, const UString& text) :
    Node(parent, text)
{
}

ts::xml::Unknown::Unknown(const Unknown& other) :
    Node(other)
{
}

ts::xml::Node* ts::xml::Unknown::clone() const
{
    return new Unknown(*this);
}

ts::UString ts::xml::Unknown::typeName() const
{
    return u"Unknown";
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Unknown::print(TextFormatter& output, bool keepNodeOpen) const
{
    // In unknown nodes, we escape all 5 XML characters: < > & ' "
    // Since the node is unknown, let's be conservative.
    output << "<!" << value().toHTML(u"<>&'\"") << ">";
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Unknown::parseNode(TextParser& parser, const Node* parent)
{
    // The current point of parsing is right after "<!", probably a DTD we do not manage.
    // The content of the node is up (but not including) the ">".

    UString content;
    bool ok = parser.parseText(content, u">", true, true);
    if (ok) {
        setValue(content);
    }
    else {
        report().error(u"line %d: error parsing unknown or DTD node, not properly terminated", {lineNumber()});
    }
    return ok;
}
