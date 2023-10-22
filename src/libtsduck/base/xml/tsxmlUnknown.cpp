//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
