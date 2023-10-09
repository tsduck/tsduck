//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlDeclaration.h"
#include "tsxmlDocument.h"
#include "tsTextFormatter.h"

// Default XML declaration.
const ts::UChar* const ts::xml::Declaration::DEFAULT_XML_DECLARATION = u"xml version=\"1.0\" encoding=\"UTF-8\"";


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Declaration::Declaration(Report& report, size_t line) :
    Node(report, line)
{
}

ts::xml::Declaration::Declaration(Document* parent, const UString& value) :
    Node(parent, value.empty() ? DEFAULT_XML_DECLARATION : value)
{
}

ts::xml::Declaration::Declaration(const Declaration& other) :
    Node(other)
{
}

ts::xml::Node* ts::xml::Declaration::clone() const
{
    return new Declaration(*this);
}

ts::UString ts::xml::Declaration::typeName() const
{
    return u"Declaration";
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Declaration::print(TextFormatter& output, bool keepNodeOpen) const
{
    output << "<?" << value() << "?>";
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Declaration::parseNode(TextParser& parser, const Node* parent)
{
    // The current point of parsing is right after "<?".
    // The content of the declaration is up (but not including) the "?>".

    UString text;
    bool ok = parser.parseText(text, u"?>", true, false);
    if (ok) {
        setValue(text);
        if (dynamic_cast<const Document*>(parent) == nullptr) {
            report().error(u"line %d: misplaced declaration, not directly inside a document", {lineNumber()});
            ok = false;
        }
    }
    else {
        report().error(u"line %d: error parsing XML declaration, not properly terminated", {lineNumber()});
    }
    return ok;
}
