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

#include "tsxmlText.h"
#include "tsxmlElement.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Text::Text(Report& report, size_t line, bool cdata, bool trimmable) :
    Node(report, line),
    _isCData(cdata),
    _trimmable(trimmable)
{
}

ts::xml::Text::Text(Element* parent, const UString& text, bool cdata, bool trimmable) :
    Node(parent, text),
    _isCData(cdata),
    _trimmable(trimmable)
{
}

ts::xml::Text::Text(const Text& other) :
    Node(other),
    _isCData(other._isCData),
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
    return !_isCData;
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Text::print(TextFormatter& output, bool keepNodeOpen) const
{
    if (_isCData) {
        output << "<![CDATA[" << value() << "]]>";
    }
    else {
        UString text(value());
        // On non-formatting output (e.g. one-liner XML text), trim all spaces when allowed.
        if (_trimmable && !output.formatting()) {
            text.trim(true, true, true);
        }
        // In text nodes, without strictly conformant XML, we escape 3 out of 5 XML characters: < > &
        // This is the required minimum to make the syntax correct.
        // The quotes (' ") are not escaped since this makes most XML text unreadable.
        text.convertToHTML(tweaks().strictTextNodeFormatting ? u"<>&'\"" : u"<>&");
        output << text;
    }
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Text::parseNode(TextParser& parser, const Node* parent)
{
    bool ok;
    UString content;

    // The current point of parsing is the first character of the text.
    if (_isCData) {
        // In the case of CDATA, we are right after the "<![CDATA[". Parse up to "]]>".
        ok = parser.parseText(content, u"]]>", true, false);
        if (ok) {
            setValue(content);
        }
        else {
            report().error(u"line %d: no ]]> found to close the <![CDATA[", {lineNumber()});
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
            report().error(u"line %d: error parsing text element, not properly terminated", {lineNumber()});
        }
    }

    return ok;
}
