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

#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsxmlParser.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::xml::Element::Element(Report& report, size_t line, CaseSensitivity attributeCase) :
    Node(report, line),
    _attributeCase(attributeCase),
    _attributes()
{
}


//----------------------------------------------------------------------------
// Clear the content of the node.
//----------------------------------------------------------------------------

void ts::xml::Element::clear()
{
    _attributes.clear();
    Node::clear();
}


//----------------------------------------------------------------------------
// Find the first child element by name, case-insensitive.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Element::findFirstChild(const UString& name, bool silent)
{
    // Loop on all children.
    for (Element* child = firstChildElement(); child != 0; child = child->nextSiblingElement()) {
        if (name.empty() || name.similar(child->name())) {
            return child;
        }
    }

    // Child node not found.
    if (!silent) {
        _report.error(u"Child node <%s> not found in <%s>, line %d", {name, _value, lineNumber()});
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find all children elements in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

bool ts::xml::Element::getChildren(ElementVector& children, const UString& searchName, size_t minCount, size_t maxCount) const
{
    children.clear();

    // Filter invalid parameters.
    if (searchName.empty()) {
        return false;
    }

    // Loop on all children.
    for (const Element* child = firstChildElement(); child != 0; child = child->nextSiblingElement()) {
        if (searchName.similar(child->name())) {
            children.push_back(child);
        }
    }

    // Check cardinality.
    if (children.size() >= minCount && children.size() <= maxCount) {
        return true;
    }
    else if (maxCount == UNLIMITED) {
        _report.error(u"<%s>, line %d, contains %d <%s>, at least %d required", {name(), lineNumber(), children.size(), searchName, minCount});
        return false;
    }
    else {
        _report.error(u"<%s>, line %d, contains %d <%s>, allowed %d to %d", {name(), lineNumber(), children.size(), searchName, minCount, maxCount});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get text in a child of an element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getTextChild(UString& data,
                                    const UString& searchName,
                                    bool trim,
                                    bool required,
                                    const UString& defValue,
                                    size_t minSize,
                                    size_t maxSize) const
{
    // Get child node.
    ElementVector child;
    if (!getChildren(child, searchName, required ? 1 : 0, 1)) {
        data.clear();
        return false;
    }

    // Get value in child node.
    if (child.empty()) {
        data = defValue;
        return true;
    }
    else {
        return child[0]->getText(data, trim, minSize, maxSize);
    }
}


//----------------------------------------------------------------------------
// Get a text child of an element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getText(UString& data, const bool trim, size_t minSize, size_t maxSize) const
{
    data.clear();

    // Locate and concatenate text children.
    for (const Node* node = firstChild(); node != 0; node = node->nextSibling()) {
        const Text* text = dynamic_cast<const Text*>(node);
        if (text != 0) {
            data.append(text->value());
        }
    }
    if (trim) {
        data.trim();
    }

    // Check value size.
    const size_t len = data.length();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        _report.error(u"Incorrect text in <%s>, line %d, contains %d characters, at least %d required", {name(), lineNumber(), len, minSize});
        return false;
    }
    else {
        _report.error(u"Incorrect text in <%s>, line %d, contains %d characters, allowed %d to %d", {name(), lineNumber(), len, minSize, maxSize});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get text in a child containing hexadecimal data.
//----------------------------------------------------------------------------

bool ts::xml::Element::getHexaTextChild(ByteBlock& data, const UString& searchName, bool required, size_t minSize, size_t maxSize) const
{
    // Get child node.
    ElementVector child;
    if (!getChildren(child, searchName, required ? 1 : 0, 1)) {
        data.clear();
        return false;
    }

    // Get value in child node.
    if (child.empty()) {
        data.clear();
        return true;
    }
    else {
        return child[0]->getHexaText(data, minSize, maxSize);
    }
}


//----------------------------------------------------------------------------
// Get a text child of an element containing hexadecimal data).
//----------------------------------------------------------------------------

bool ts::xml::Element::getHexaText(ByteBlock& data, size_t minSize, size_t maxSize) const
{
    data.clear();

    // Get text children.
    UString text;
    if (!getText(text)) {
        return false;
    }

    // Interpret hexa data.
    if (!text.hexaDecode(data)) {
        _report.error(u"Invalid hexadecimal content in <%s>, line %d", {name(), lineNumber()});
        return false;
    }

    // Check value size.
    const size_t len = data.size();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        _report.error(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, at least %d required", {name(), lineNumber(), len, minSize});
        return false;
    }
    else {
        _report.error(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, allowed %d to %d", {name(), lineNumber(), len, minSize, maxSize});
        return false;
    }
}


//----------------------------------------------------------------------------
// Attribute map management.
//----------------------------------------------------------------------------

ts::UString ts::xml::Element::attributeKey(const UString& attributeName) const
{
    return _attributeCase == CASE_SENSITIVE ? attributeName : attributeName.toLower();
}

ts::xml::Element::AttributeMap::const_iterator ts::xml::Element::findAttribute(const UString& attributeName) const
{
    return _attributes.find(attributeKey(attributeName));
}

void ts::xml::Element::setAttribute(const UString& name, const UString& value)
{
    _attributes[attributeKey(name)] = Attribute(name, value);
}

bool ts::xml::Element::hasAttribute(const UString& name) const
{
    return findAttribute(name) != _attributes.end();
}

const ts::xml::Attribute& ts::xml::Element::attribute(const UString& attributeName) const
{
    const AttributeMap::const_iterator it(findAttribute(attributeName));
    return it == _attributes.end() ? Attribute::INVALID : it->second;
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Element::parseNode(Parser& parser, const Node* parent)
{
    // We just read the "<". Skip spaces and read the tag name.
    parser.skipWhiteSpace();
    if (!parser.parseName(_value)) {
        _report.error(u"line %d: parsing error, tag name expected", {parser.lineNumber()});
        return false;
    }

    // Read the list of attributes.
    bool ok = true;
    while (ok) {
        UString name;
        UString value;
        UChar* quote = 0;

        parser.skipWhiteSpace();

        if (parser.match(u">", true)) {
            // Found end of tag.
            break;
        }
        else if (parser.match(u"/>", true)) {
            // Found end of standalone tag, without children.
            return true;
        }
        else if (parser.parseName(name)) {
            // Found a name, probably an attribute.
            const size_t line = parser.lineNumber();

            // Expect '='.
            parser.skipWhiteSpace();
            ok = parser.match(u"=", true);
            if (ok) {
                parser.skipWhiteSpace();
                if (parser.match(u"\"", true)) {
                    quote = u"\"";
                }
                else if (parser.match(u"'", true)) {
                    quote = u"'";
                }
                else {
                    ok = false;
                }
            }

            // Read attribute value.
            ok = ok && parser.parseText(value, quote, true, true);

            // Store the attribute.
            if (!ok) {
                _report.error(u"line %d: error parsing attribute '%s' in tag <%s>", {line, name, _value});
            }
            else if (hasAttribute(name)) {
                _report.error(u"line %d: duplicate attribute '%s' in tag <%s>", {line, name, _value});
                ok = false;
            }
            else {
                _attributes[attributeKey(name)] = Attribute(name, value, line);
            }
        }
        else {
            _report.error(u"line %d: parsing error, tag <%s>", {lineNumber(), _value});
            ok = false;
        }
    }

    // In case of error inside the tag, try to locate the end of tag.
    // There is no guarantee that the parsing may continue further however.
    if (!ok) {
        UString ignored;
        parser.parseText(ignored, u">", true, false);
        return false;
    }

    // End of tag, swallow all children.
    if (!parseChildren(parser)) {
        return false;
    }

    // We now must be at "</tag>".
    ok = parser.match(u"</", true);
    if (ok) {
        UString endTag;
        ok = parser.skipWhiteSpace() && parser.parseName(endTag) && parser.skipWhiteSpace() && endTag.similar(_value);
        ok = parser.match(u">", true) && ok;
    }

    if (!ok) {
        _report.error(u"line %d: parsing error, expected </%s> to match <%s> at line %d", {parser.lineNumber(), _value, _value, lineNumber()});
    }

    return ok;
}
