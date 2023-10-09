//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlComment.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Comment::Comment(Report& report, size_t line) :
    Node(report, line)
{
}

ts::xml::Comment::Comment(Node* parent, const UString& text, bool last) :
    Node(parent, text, last)
{
}

ts::xml::Comment::Comment(const Comment& other) :
    Node(other)
{
}

ts::xml::Node* ts::xml::Comment::clone() const
{
    return new Comment(*this);
}

ts::UString ts::xml::Comment::typeName() const
{
    return u"Comment";
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Comment::print(TextFormatter& output, bool keepNodeOpen) const
{
    output << "<!--" << value() << "-->";
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Comment::parseNode(TextParser& parser, const Node* parent)
{
    // The current point of parsing is right after "<!--".
    // The content of the comment is up (but not including) the "-->".

    UString content;
    bool ok = parser.parseText(content, u"-->", true, false);
    if (ok) {
        setValue(content);
    }
    else {
        report().error(u"line %d: error parsing XML comment, not properly terminated", {lineNumber()});
    }
    return ok;
}
