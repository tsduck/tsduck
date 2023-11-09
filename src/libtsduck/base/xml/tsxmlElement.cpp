//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::xml::Element::Element(Report& report, size_t line, CaseSensitivity attributeCase) :
    Node(report, line),
    _attributeCase(attributeCase)
{
}

ts::xml::Element::Element(Node* parent, const UString& name, CaseSensitivity attributeCase, bool last) :
    Node(parent, name, last), // the "value" of an element node is its name.
    _attributeCase(attributeCase)
{
}

ts::xml::Element::Element(const Element& other) :
    Node(other),
    _attributeCase(other._attributeCase),
    _attributes(other._attributes)
{
}

ts::xml::Node* ts::xml::Element::clone() const
{
    return new Element(*this);
}

ts::UString ts::xml::Element::typeName() const
{
    return u"Element";
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
        report().error(u"Child node <%s> not found in <%s>, line %d", {name, value(), lineNumber()});
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
        report().error(u"<%s>, line %d, contains %d <%s>, at least %d required", {name(), lineNumber(), children.size(), searchName, minCount});
        return false;
    }
    else {
        report().error(u"<%s>, line %d, contains %d <%s>, allowed %d to %d", {name(), lineNumber(), children.size(), searchName, minCount, maxCount});
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
        report().error(u"Incorrect text in <%s>, line %d, contains %d characters, at least %d required", {name(), lineNumber(), len, minSize});
        return false;
    }
    else {
        report().error(u"Incorrect text in <%s>, line %d, contains %d characters, allowed %d to %d", {name(), lineNumber(), len, minSize, maxSize});
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

    // Get text children. Ignore errors if no text found, simply empty.
    // Interpret hexa data.
    if (!text().hexaDecode(data)) {
        report().error(u"Invalid hexadecimal content in <%s>, line %d", {name(), lineNumber()});
        return false;
    }

    // Check value size.
    const size_t len = data.size();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        report().error(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, at least %d required", {name(), lineNumber(), len, minSize});
        return false;
    }
    else {
        report().error(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, allowed %d to %d", {name(), lineNumber(), len, minSize, maxSize});
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

ts::xml::Text* ts::xml::Element::addText(const UString& text, bool onlyNotEmpty)
{
    if (onlyNotEmpty && text.empty()) {
        return nullptr;
    }
    else {
        Text* child = new Text(this, text);
        CheckNonNull(child);
        return child;
    }
}


//----------------------------------------------------------------------------
// Add a new text containing hexadecimal data inside this node.
//----------------------------------------------------------------------------

ts::xml::Text* ts::xml::Element::addHexaText(const void* data, size_t size, bool onlyNotEmpty)
{
    // Filter incorrect parameters.
    if (data == nullptr) {
        data = "";
        size = 0;
    }

    // Do nothing if empty.
    if (size == 0 && onlyNotEmpty) {
        return nullptr;
    }

    // Format the data.
    const size_t dep = depth();
    const UString hex(UString::Dump(data, size, UString::HEXA | UString::BPL, 2 * dep, 16));

    // Add the text node. Try to indent it in a nice way.
    xml::Text* text = addText(u"\n" + hex + UString(dep == 0 ? 0 : 2 * (dep - 1), u' '));

    // Despite the nice indentation, hexa text can be trimmed when necessary.
    text->setTrimmable(true);

    return text;
}


//----------------------------------------------------------------------------
// Add a new child element containing an hexadecimal data text.
//----------------------------------------------------------------------------

ts::xml::Text* ts::xml::Element::addHexaTextChild(const UString& name, const void* data, size_t size, bool onlyNotEmpty)
{
    if (data == nullptr) {
        size = 0;
    }
    return size == 0 && onlyNotEmpty ? nullptr : addElement(name)->addHexaText(data, size);
}

ts::xml::Text* ts::xml::Element::addHexaTextChild(const UString& name, const ByteBlock& data, bool onlyNotEmpty)
{
    return data.empty() && onlyNotEmpty ? nullptr : addElement(name)->addHexaText(data.data(), data.size());
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

void ts::xml::Element::setAttribute(const UString& name, const UString& value, bool onlyIfNotEmpty)
{
    if (!onlyIfNotEmpty || !value.empty()) {
        _attributes[attributeKey(name)] = Attribute(name, value);
    }
}

void ts::xml::Element::deleteAttribute(const UString& name)
{
    const auto it = _attributes.find(attributeKey(name));
    if (it != _attributes.end()) {
        _attributes.erase(it);
    }
}

bool ts::xml::Element::hasAttribute(const UString& name) const
{
    return findAttribute(name) != _attributes.end();
}

ts::xml::Attribute& ts::xml::Element::refAttribute(const UString& name)
{
    const auto it = _attributes.find(attributeKey(name));
    return it == _attributes.end() ? (_attributes[attributeKey(name)] = Attribute(name, u"")) : it->second;
}


//----------------------------------------------------------------------------
// Get an attribute.
//----------------------------------------------------------------------------

const ts::xml::Attribute& ts::xml::Element::attribute(const UString& attributeName, bool silent) const
{
    const auto it = findAttribute(attributeName);
    if (it != _attributes.end()) {
        // Found the real attribute.
        return it->second;
    }
    if (!silent) {
        report().error(u"attribute '%s' not found in <%s>, line %d", {attributeName, name(), lineNumber()});
    }
    // Return a reference to a static invalid attribute.
    return Attribute::INVALID;
}


//----------------------------------------------------------------------------
// Check if an attribute exists in the element and has the specified value.
//----------------------------------------------------------------------------

bool ts::xml::Element::hasAttribute(const UString& name, const UString& value, bool similar) const
{
    const Attribute& attr(attribute(name, true));
    if (!attr.isValid()) {
        // Attribute not present.
        return false;
    }
    else if (similar) {
        return value.similar(attr.value());
    }
    else {
        return value == attr.value();
    }
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
            report().error(u"Incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, at least %d required",
                          {name, this->name(), attr.lineNumber(), value.length(), minSize});
            return false;
        }
        else {
            report().error(u"Incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, allowed %d to %d",
                          {name, this->name(), attr.lineNumber(), value.length(), minSize, maxSize});
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Get an optional string attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getOptionalAttribute(std::optional<UString>& value, const UString& name, size_t minSize, size_t maxSize) const
{
    // Default: erase value.
    value.reset();
    bool ok = true;

    if (hasAttribute(name)) {
        // Attribute present, value must be correct.
        UString val;
        ok = getAttribute(val, name, true, UString(), minSize, maxSize);
        if (ok) {
            value = val;
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Get a boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getBoolAttribute(bool& value, const UString& name, bool required, bool defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    else if (!required && str.empty()) {
        value = defValue;
        return true;
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
        report().error(u"'%s' is not a valid boolean value for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an optional boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getOptionalBoolAttribute(std::optional<bool>& value, const UString& name) const
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
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = defValue;
        return !required;
    }
    else {
        // Attribute found, get its value.
        const UString str(attr.value());
        const int val = definition.value(str, false);
        if (val == Enumeration::UNKNOWN) {
            report().error(u"'%s' is not a valid value for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
            return false;
        }
        else {
            value = val;
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Get a date/time attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getDateTimeAttribute(Time& value, const UString& name, bool required, const Time& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    // Analyze the time string.
    const bool ok = Attribute::DateTimeFromString(value, str);
    if (!ok) {
        report().error(u"'%s' is not a valid date/time for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD hh:mm:ss\"", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a date attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getDateAttribute(Time& value, const UString& name, bool required, const Time& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    // Analyze the time string.
    const bool ok = Attribute::DateFromString(value, str);
    if (!ok) {
        report().error(u"'%s' is not a valid date for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD\"", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a time attribute of an XML element in "hh:mm:ss" format.
//----------------------------------------------------------------------------

bool ts::xml::Element::getTimeAttribute(Second& value, const UString& name, bool required, Second defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    // Analyze the time string.
    const bool ok = Attribute::TimeFromString(value, str);
    if (!ok) {
        report().error(u"'%s' is not a valid time for attribute '%s' in <%s>, line %d, use \"hh:mm:ss\"", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get an IPv4/v6 or MAC address attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getIPAttribute(IPv4Address& value, const UString& name, bool required, const IPv4Address& defValue) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = defValue;
        return true;
    }

    const bool ok = value.resolve(str, report());
    if (!ok) {
        report().error(u"'%s' is not a valid IPv4 address for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
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

    const bool ok = value.resolve(str, report());
    if (!ok) {
        report().error(u"'%s' is not a valid IPv6 address for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
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

    const bool ok = value.resolve(str, report());
    if (!ok) {
        report().error(u"'%s' is not a valid MAC address for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get the list of all attribute names.
//----------------------------------------------------------------------------

void ts::xml::Element::getAttributesNames(UStringList& names) const
{
    names.clear();
    for (const auto& it : _attributes) {
        names.push_back(it.second.name());
    }
}


//----------------------------------------------------------------------------
// Get the list of all attributes.
//----------------------------------------------------------------------------

void ts::xml::Element::getAttributes(std::map<UString,UString>& attr) const
{
    attr.clear();
    for (const auto& it : _attributes) {
        attr[it.first] = it.second.value();
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
    for (const auto& it : _attributes) {
        nameMap.insert(std::make_pair(it.second.sequence(), it.second.name()));
    }

    // Then build the name list, ordered by sequence number.
    names.clear();
    for (const auto& it : nameMap) {
        names.push_back(it.second);
    }
}


//----------------------------------------------------------------------------
// Recursively merge another element into this one.
//----------------------------------------------------------------------------

bool ts::xml::Element::merge(Element* other, MergeAttributes attrOptions)
{
    // Ignore null or self merge.
    if (other == nullptr || other == this) {
        return true;
    }

    // Check that the elements have identical tags.
    if (!name().similar(other->name())) {
        report().error(u"Cannot merge XML element <%s>, line %d, with <%s>, line %d", {name(), lineNumber(), other->name(), other->lineNumber()});
        return false;
    }

    // Merge attributes.
    if (attrOptions != MergeAttributes::NONE) {
        for (const auto& attr : other->_attributes) {
            if (attrOptions == MergeAttributes::REPLACE || !hasAttribute(attr.second.name())) {
                setAttribute(attr.second.name(), attr.second.value());
            }
        }
    }

    // Remove elements one by one from the node to merge.
    xml::Element* elem = nullptr;
    while ((elem = other->firstChildElement()) != nullptr) {
        // We need to merge its content with an element of the same name in the main.
        xml::Element* main = findFirstChild(elem->name(), true);
        if (main == nullptr) {
            // The tag did not exist in the main element, simply move is here.
            elem->reparent(this);
        }
        else {
            // Move all content into the main topic.
            main->merge(elem, attrOptions);
        }
    }

    // Finally, delete the (now empty) merged element.
    delete other;
    return true;
}


//----------------------------------------------------------------------------
// Sort children elements by alphabetical order of tag name.
//----------------------------------------------------------------------------

void ts::xml::Element::sort(const UString& tagName)
{
    // Sort children in current element.
    if (tagName.empty() || tagName.similar(name())) {
        Element* child = firstChildElement();
        while (child != nullptr) {
            Element* next = child->nextSiblingElement();

            // Go backward until we find a "pos" where to insert "child".
            Element* prev = nullptr;
            Element* pos = child;
            while ((prev = pos->previousSiblingElement()) != nullptr && prev->name() > child->name()) {
                pos = prev;
            }
            if (pos != child && pos != nullptr) {
                child->moveBefore(pos);
            }

            child = next;
        }
    }

    // Recursively sort children.
    if (!tagName.empty()) {
        for (Element* child = firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
            child->sort(tagName);
        }
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
    for (const auto& atname : names) {
        const Attribute& attr(attribute(atname));
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
            output << ts::endl << ts::margin;
        }
        node->print(output, false);
    }

    // Close the element if required.
    if (!sticky || keepNodeOpen) {
        output << ts::endl;
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
        output << ts::unindent << ts::margin << "</" << elem->name() << ">" << ts::endl;
    }
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Element::parseNode(TextParser& parser, const Node* parent)
{
    // We just read the "<". Skip spaces and read the tag name.
    UString nodeName;
    parser.skipWhiteSpace();
    if (!parser.parseXMLName(nodeName)) {
        report().error(u"line %d: parsing error, tag name expected", {parser.lineNumber()});
        return false;
    }

    // The "value" of an element is its tag name.
    setValue(nodeName);

    // Read the list of attributes.
    bool ok = true;
    while (ok) {
        UString attrName;
        UString attrValue;
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
        else if (parser.parseXMLName(attrName)) {
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
            ok = ok && parser.parseText(attrValue, quote, true, true);

            // Store the attribute.
            if (!ok) {
                report().error(u"line %d: error parsing attribute '%s' in tag <%s>", {line, attrName, value()});
            }
            else if (hasAttribute(attrName)) {
                report().error(u"line %d: duplicate attribute '%s' in tag <%s>", {line, attrName, value()});
                ok = false;
            }
            else {
                _attributes[attributeKey(attrName)] = Attribute(attrName, attrValue, line);
            }
        }
        else {
            report().error(u"line %d: parsing error, tag <%s>", {lineNumber(), value()});
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
        ok = parser.skipWhiteSpace() && parser.parseXMLName(endTag) && parser.skipWhiteSpace() && endTag.similar(value());
        ok = parser.match(u">", true) && ok;
    }

    if (!ok) {
        report().error(u"line %d: parsing error, expected </%s> to match <%s> at line %d", {parser.lineNumber(), value(), value(), lineNumber()});
    }

    return ok;
}
