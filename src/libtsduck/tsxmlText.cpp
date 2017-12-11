//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsxmlParser.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::xml::Text::Text(size_t line, bool cdata) :
    Node(line),
    _isCData(cdata)
{
}


//----------------------------------------------------------------------------
// Continue the parsing of a document from the point where this node start up
// to the end. This is the Text implementation.
//----------------------------------------------------------------------------

bool ts::xml::Text::parseContinue(Parser& parser, UString& endToken)
{
    bool ok;

    // The current point of parsing is the first character of the text.
    if (_isCData) {
        // In the case of CDATA, we right after the "<![CDATA[". Parse up to "]]>".
        ok = parser.parseText(_value, u"]]>", false);
        if (!ok) {
            parser.errorAtLine(lineNumber(), u"no ]]> found to close the <![CDATA[", {});
        }
    }
    else {
        // Outside CDATA, the text ends at the next "<" (start of tag).
        // HTML entities shall be translated.
        ok = parser.parseText(_value, u"<", true);
        if (!ok) {
            parser.errorAtLine(lineNumber(), u"error parsing text element, not properly terminated", {});
        }
    }

    return ok;
}
