//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsFatal.h"
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

ts::xml::Element::Element(Node* parent, const UString& name, CaseSensitivity attributeCase) :
    Node(parent, name), // the "value" of an element node is its name.
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
    for (Element* child = firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (name.empty() || name.similar(child->name())) {
            return child;
        }
    }

    // Child node not found.
    if (!silent) {
        _report.error(u"Child node <%s> not found in <%s>, line %d", {name, _value, lineNumber()});
    }
    return nullptr;
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
    for (const Element* child = firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
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

ts::UString ts::xml::Element::text(bool trim) const
{
    UString str;
    getText(str, trim);
    return str;
}

bool ts::xml::Element::getText(UString& data, const bool trim, size_t minSize, size_t maxSize) const
{
    data.clear();

    // Locate and concatenate text children.
    for (const Node* node = firstChild(); node != nullptr; node = node->nextSibling()) {
        const Text* text = dynamic_cast<const Text*>(node);
        if (text != nullptr) {
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
// Add a new child element at the end of children.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Element::addElement(const UString& childName)
{
    Element* child = new Element(this, childName);
    CheckNonNull(child);
    return child;
}


//----------------------------------------------------------------------------
// Add a new text inside this node.
//----------------------------------------------------------------------------

ts::xml::Text* ts::xml::Element::addText(const UString& text)
{
    Text* child = new Text(this, text);
    CheckNonNull(child);
    return child;
}


//----------------------------------------------------------------------------
// Add a new text containing hexadecimal data inside this node.
//----------------------------------------------------------------------------

ts::xml::Text* ts::xml::Element::addHexaText(const void* data, size_t size)
{
    // Filter incorrect parameters.
    if (data == nullptr) {
        data = "";
        size = 0;
    }

    // Format the data.
    const size_t dep = depth();
    const UString hex(UString::Dump(data, size, UString::HEXA | UString::BPL, 2 * dep, 16));

    // Add the text node. Try to indent it in a nice way.
    return addText(u"\n" + hex + UString(dep == 0 ? 0 : 2 * (dep - 1), u' '));
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

ts::xml::Attribute& ts::xml::Element::refAttribute(const UString& name)
{
    const AttributeMap::iterator it(_attributes.find(attributeKey(name)));
    return it == _attributes.end() ? (_attributes[attributeKey(name)] = Attribute(name, u"")) : it->second;
}


//----------------------------------------------------------------------------
// Get an attribute.
//----------------------------------------------------------------------------

const ts::xml::Attribute& ts::xml::Element::attribute(const UString& attributeName, bool silent) const
{
    const AttributeMap::const_iterator it(findAttribute(attributeName));
    if (it != _attributes.end()) {
        // Found the real attribute.
        return it->second;
    }
    if (!silent) {
        _report.error(u"attribute '%s' not found in <%s>, line %d", {attributeName, name(), lineNumber()});
    }
    // Return a reference to a static invalid attribute.
    return Attribute::INVALID;
}


//----------------------------------------------------------------------------
// Get a string attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getAttribute(UString& value,
                                    const UString& name,
                                    bool required,
                                    const UString& defValue,
                                    size_t minSize,
                                    size_t maxSize) const
{
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = defValue;
        return !required;
    }
    else {
        // Attribute found, get its value.
        value = attr.value();
        if (value.length() >= minSize && value.length() <= maxSize) {
            return true;
        }

        // Incorrect value size.
        if (maxSize == UNLIMITED) {
            _report.error(u"Incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, at least %d required",
                          {name, this->name(), attr.lineNumber(), value.length(), minSize});
            return false;
        }
        else {
            _report.error(u"Incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, allowed %d to %d",
                          {name, this->name(), attr.lineNumber(), value.length(), minSize, maxSize});
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Get a boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getBoolAttribute(bool& value, const UString& name, bool required, bool defValue) const
{
    UString str;
    if (!getAttribute(str, name, required, UString::TrueFalse(defValue))) {
        return false;
    }
    else if (str.similar(u"true") || str.similar(u"yes") || str.similar(u"1")) {
        value = true;
        return true;
    }
    else if (str.similar(u"false") || str.similar(u"no") || str.similar(u"0")) {
        value = false;
        return true;
    }
    else {
        _report.error(u"'%s' is not a valid boolean value for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an optional boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getOptionalBoolAttribute(Variable<bool>& value, const UString& name) const
{
    // Default: erase value.
    value.reset();
    bool ok = true;

    if (hasAttribute(name)) {
        // Attribute present, value must be correct.
        bool val = false;
        ok = getBoolAttribute(val, name, true);
        if (ok) {
            value = val;
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Get an enumeration attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getEnumAttribute(int& value, const Enumeration& definition, const UString& name, bool required, int defValue) const
{
    UString str;
    if (!getAttribute(str, name, required, UString::Decimal(defValue))) {
        return false;
    }
    const int val = definition.value(str, false);
    if (val == Enumeration::UNKNOWN) {
        _report.error(u"'%s' is not a valid value for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
        return false;
    }
    else {
        value = val;
        return true;
    }
}


//----------------------------------------------------------------------------
// Get a date/time attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getDateTimeAttribute(Time& value, const UString& name, bool required, const Time& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required, Attribute::DateTimeToString(defValue))) {
        return false;
    }

    // Analyze the time string.
    const bool ok = Attribute::DateTimeFromString(value, str);
    if (!ok) {
        _report.error(u"'%s' is not a valid date/time for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD hh:mm:ss\"", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a time attribute of an XML element in "hh:mm:ss" format.
//----------------------------------------------------------------------------

bool ts::xml::Element::getTimeAttribute(Second& value, const UString& name, bool required, Second defValue) const
{
    UString str;
    if (!getAttribute(str, name, required, Attribute::TimeToString(defValue))) {
        return false;
    }

    // Analyze the time string.
    const bool ok = Attribute::TimeFromString(value, str);
    if (!ok) {
        _report.error(u"'%s' is not a valid time for attribute '%s' in <%s>, line %d, use \"hh:mm:ss\"", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get an IPv4/v6 or MAC address attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getIPAttribute(IPAddress& value, const UString& name, bool required, const IPAddress& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    const bool ok = value.resolve(str, _report);
    if (!ok) {
        _report.error(u"'%s' is not a valid IPv4 address for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
    }
    return ok;
}

bool ts::xml::Element::getIPv6Attribute(IPv6Address& value, const UString& name, bool required, const IPv6Address& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    const bool ok = value.resolve(str, _report);
    if (!ok) {
        _report.error(u"'%s' is not a valid IPv6 address for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
    }
    return ok;
}

bool ts::xml::Element::getMACAttribute(MACAddress& value, const UString& name, bool required, const MACAddress& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    const bool ok = value.resolve(str, _report);
    if (!ok) {
        _report.error(u"'%s' is not a valid MAC address for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get the list of all attribute names.
//----------------------------------------------------------------------------

void ts::xml::Element::getAttributesNames(UStringList& names) const
{
    names.clear();
    for (AttributeMap::const_iterator it = _attributes.begin(); it != _attributes.end(); ++it) {
        names.push_back(it->second.name());
    }
}


//----------------------------------------------------------------------------
// Get the list of all attribute names, sorted by modification order.
//----------------------------------------------------------------------------

void ts::xml::Element::getAttributesNamesInModificationOrder(UStringList& names) const
{
    // Map of names, indexed by sequence number.
    typedef std::multimap<size_t, UString> NameMap;
    NameMap nameMap;

    // Read all names and build a map indexed by sequence number.
    for (AttributeMap::const_iterator it = _attributes.begin(); it != _attributes.end(); ++it) {
        nameMap.insert(std::make_pair(it->second.sequence(), it->second.name()));
    }

    // Then build the name list, ordered by sequence number.
    names.clear();
    for (NameMap::const_iterator it = nameMap.begin(); it != nameMap.end(); ++it) {
        names.push_back(it->second);
    }
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Element::print(TextFormatter& output, bool keepNodeOpen) const
{
    // Output element name.
    output << "<" << name();

    // Get all attributes names, by modification order.
    UStringList names;
    getAttributesNamesInModificationOrder(names);

    // Loop on all attributes.
    for (UStringList::const_iterator it = names.begin(); it != names.end(); ++it) {
        const Attribute& attr(attribute(*it));
        output << " " << attr.name() << "=" << attr.formattedValue(tweaks());
    }

    // Close the tag and return if nothing else to output.
    if (!hasChildren() && !keepNodeOpen) {
        output << "/>";
        return;
    }

    // Keep the tag open for children.
    output << ">";

    output << ts::indent;
    bool sticky = false;

    // Display list of children.
    for (const Node* node = firstChild(); node != nullptr; node = node->nextSibling()) {
        const bool previousSticky = sticky;
        sticky = node->stickyOutput();
        if (!previousSticky && !sticky) {
            output << std::endl << ts::margin;
        }
        node->print(output, false);
    }

    // Close the element if required.
    if (!sticky || keepNodeOpen) {
        output << std::endl;
    }
    if (!keepNodeOpen) {
        output << ts::unindent;
        if (!sticky) {
            output << ts::margin;
        }
        output << "</" << name() << ">";
    }
}


//----------------------------------------------------------------------------
// Print the closing tags for a node.
//----------------------------------------------------------------------------

void ts::xml::Element::printClose(TextFormatter& output, size_t levels) const
{
    for (const Element* elem = this; levels-- > 0 && elem != nullptr; elem = dynamic_cast<const Element*>(elem->parent())) {
        output << ts::unindent << ts::margin << "</" << elem->name() << ">" << std::endl;
    }
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Element::parseNode(TextParser& parser, const Node* parent)
{
    // We just read the "<". Skip spaces and read the tag name.
    parser.skipWhiteSpace();
    if (!parser.parseXMLName(_value)) {
        _report.error(u"line %d: parsing error, tag name expected", {parser.lineNumber()});
        return false;
    }

    // Read the list of attributes.
    bool ok = true;
    while (ok) {
        UString name;
        UString value;
        const UChar* quote = nullptr;

        parser.skipWhiteSpace();

        if (parser.match(u">", true)) {
            // Found end of tag.
            break;
        }
        else if (parser.match(u"/>", true)) {
            // Found end of standalone tag, without children.
            return true;
        }
        else if (parser.parseXMLName(name)) {
            // Found a name, probably an attribute.
            const size_t line = parser.lineNumber();

            // Expect '='.
            parser.skipWhiteSpace();
            ok = parser.match(u"=", true);

            // Expect either single or double quote. Both can be used for the attribute value.
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
        ok = parser.skipWhiteSpace() && parser.parseXMLName(endTag) && parser.skipWhiteSpace() && endTag.similar(_value);
        ok = parser.match(u">", true) && ok;
    }

    if (!ok) {
        _report.error(u"line %d: parsing error, expected </%s> to match <%s> at line %d", {parser.lineNumber(), _value, _value, lineNumber()});
    }

    return ok;
}
