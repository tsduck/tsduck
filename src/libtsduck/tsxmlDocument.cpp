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

#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsxmlDeclaration.h"
#include "tsxmlParser.h"
#include "tsReportWithPrefix.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::xml::Document::parse(const UStringList& lines)
{
    Parser parser(lines, _report);
    return parseNode(parser, 0);
}


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::xml::Document::parse(const UString& text)
{
    UStringList lines;
    text.split(lines, u'\n', false);
    return parse(lines);
}


//----------------------------------------------------------------------------
// Load and parse an XML file.
//----------------------------------------------------------------------------

bool ts::xml::Document::load(const UString& fileName)
{
    UStringList lines;
    if (UString::Load(lines, fileName)) {
        _report.error(u"error reading file %s", {fileName});
        return false;
    }
    else {
        setReportPrefix(fileName + u": ");
        const bool ok = parse(lines);
        setReportPrefix(u"");
        return ok;
    }
}

//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Document::parseNode(Parser& parser, const Node* parent)
{
    // The document is a simple list of children.
    if (!parseChildren(parser)) {
        return false;
    }
    std::cerr << "document : " << debug() << std::endl; //@@@@

    // We must have reached the end of document.
    if (!parser.eof()) {
        _report.error(u"line %d: trailing character sequence, invalid XML document", {parser.lineNumber()});
        return false;
    }

    // A document must contain optional declarations, followed by one single element (the root).
    Node* child = firstChild();

    // First, skip all leading declarations.
    while (dynamic_cast<Declaration*>(child) != 0) {
        child = child->nextSibling();
    }

    // Check presence of root element.
    if (dynamic_cast<Element*>(child) == 0) {
        std::cerr << "document root: " << (child == 0 ? u"null" : child->debug()) << std::endl; //@@@@
        _report.error(u"invalid XML document, no root element found");
        return false;
    }

    // Skip root element.
    child = child->nextSibling();

    // Verify that there is no additional children.
    if (child != 0) {
        _report.error(u"line %d: trailing %s, invalid XML document, need one single root element", {child->lineNumber(), child->typeName()});
        return false;
    }

    // Valid document.
    return true;
}
