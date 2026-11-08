//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlText.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Text::Text(Report& report, size_t line, bool cdata, bool trimmable) :
    Node(report, line),
    _is_cdata(cdata),
    _trimmable(trimmable)
{
}

ts::xml::Text::Text(Element* parent, const UString& text, bool cdata, bool trimmable) :
    Node(parent, text),
    _is_cdata(cdata),
    _trimmable(trimmable)
{
}

ts::xml::Text::Text(const Text& other) :
    Node(other),
    _is_cdata(other._is_cdata),
    _trimmable(other._trimmable)
{
}

ts::xml::Node* ts::xml::Text::clone() const
{
    return new Text(*this);
}

ts::UString ts::xml::Text::typeName() const
{
    return u"Text";
}

bool ts::xml::Text::stickyOutput() const
{
    return !_is_cdata;
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Text::print(TextFormatter& output, bool keep_node_open) const
{
    if (_is_cdata) {
        // In a CDATA, the square brackets must also be escaped.
        output << "<![CDATA[" << escapedString(value(), u"[]", false) << "]]>";
    }
    else {
        // On non-formatting output (e.g. one-liner XML text), trim all spaces when allowed.
        output << escapedString(value(), u"", _trimmable && !output.formatting());
    }
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Text::parseNode(TextParser& parser, const Node* parent, size_t max_nested)
{
    bool ok;
    UString content;

    // The current point of parsing is the first character of the text.
    if (_is_cdata) {
        // In the case of CDATA, we are right after the "<![CDATA[". Parse up to "]]>".
        ok = parser.parseText(content, u"]]>", true, false);
        if (ok) {
            setValue(content);
        }
        else {
            report().error(u"line %d: no ]]> found to close the <![CDATA[", lineNumber());
        }
    }
    else {
        // Outside CDATA, the text ends at the next "<" (start of tag).
        // HTML entities shall be translated.
        ok = parser.parseText(content, u"<", false, true);
        if (ok) {
            setValue(content);
        }
        else {
            report().error(u"line %d: error parsing text element, not properly terminated", lineNumber());
        }
    }

    return ok;
}
