//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsBase64.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::xml::Element::Element(Report& report, size_t line) :
    Node(report, line)
{
}

ts::xml::Element::Element(Node* parent, const UString& name, bool last) :
    Node(parent, name, last) // the "value" of an element node is its name.
{
}

ts::xml::Element::Element(const Element& other) :
    Node(other),
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
// Get the parent name.
//----------------------------------------------------------------------------

const ts::UString& ts::xml::Element::parentName() const
{
    return parent() == nullptr ? UString::EMPTY() : parent()->value();
}


//----------------------------------------------------------------------------
// Specify if namespace is ignored by default when comparing names.
//----------------------------------------------------------------------------

void ts::xml::Element::setIignoreNamespace(bool ignore)
{
    // Costly recursive operation, do it only when necessary.
    if (ignoreNamespace() != ignore) {

        // Call the superclass to set the node's property.
        Node::setIignoreNamespace(ignore);

        // Set the property on all XML attributes.
        for (auto& it : _attributes) {
            it.second.setIignoreNamespace(ignore);
        }
    }
}


//----------------------------------------------------------------------------
// Check if the name of the element matches a given value, case-insensitive.
//----------------------------------------------------------------------------

bool ts::xml::Element::nameMatch(const UChar* str, bool ignore_namespace) const
{
    return str != nullptr && (ignore_namespace ? name().similarAfterLast(str, u':') : name().similar(str));
}


//----------------------------------------------------------------------------
// Expand all environment variables in the XML node.
//----------------------------------------------------------------------------

void ts::xml::Element::expandEnvironment(bool recurse)
{
    // Expand in attributes values.
    for (auto& it : _attributes) {
        it.second.expandEnvironment();
    }

    // Call superclass
    Node::expandEnvironment(recurse);
}


//----------------------------------------------------------------------------
// Iterating over a constant list of XML elements
//----------------------------------------------------------------------------

// Iterator pre-decrement operator.
ts::xml::Element::ConstElementIterator& ts::xml::Element::ConstElementIterator::operator--()
{
    assert(_set != nullptr);
    if (_set->isValid()) {
        // Iterating over associated set is valid, pre-decrement superclass.
        --*static_cast<SuperClass*>(this);
    }
    else {
        // Iterating over associated set has been invalidated, always point to end, will terminate iterations.
        *static_cast<SuperClass*>(this) = _set->_elements.end();
    }
    return *this;
}

// Iterator pre-increment operator.
ts::xml::Element::ConstElementIterator& ts::xml::Element::ConstElementIterator::operator++()
{
    assert(_set != nullptr);
    if (_set->isValid()) {
        // Iterating over associated set is valid, pre-increment superclass.
        ++*static_cast<SuperClass*>(this);
    }
    else {
        // Iterating over associated set has been invalidated, always point to end, will terminate iterations.
        *static_cast<SuperClass*>(this) = _set->_elements.end();
    }
    return *this;
}

// Get the iterator to the beginning of the set.
ts::xml::Element::ConstElementIterator ts::xml::Element::ConstElementSet::begin() const
{
    // If iteration is invalidated, return end() to prevent iteration.
    return ConstElementIterator(*this, isValid() ? _elements.begin() : _elements.end());
}

// Get an iterable set of all children elements of a given name.
ts::xml::Element::ConstElementSet ts::xml::Element::children(const UString& search_name, bool* valid_condition, size_t min_count, size_t max_count) const
{
    ConstElementSet set;

    // If condition is already invalid, search nothing.
    set._valid = valid_condition;
    if (set.isValid()) {

        // Search all matching children and stores them in the set.
        for (const Element* child = firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
            if (search_name.empty() || child->nameMatch(search_name)) {
                set._elements.push_back(child);
            }
        }

        // Check cardinality.
        const size_t size = set._elements.size();
        if (size < min_count || size > max_count) {
            // Report error.
            if (max_count == UNLIMITED) {
                report().error(u"<%s>, line %d, contains %d <%s>, at least %d required", name(), lineNumber(), size, search_name, min_count);
            }
            else {
                report().error(u"<%s>, line %d, contains %d <%s>, allowed %d to %d", name(), lineNumber(), size, search_name, min_count, max_count);
            }
            // Prevent iteration.
            set._elements.clear();
            // Enforce the set as "invalid", even without explicit valid_condition.
            if (valid_condition != nullptr) {
                *valid_condition = false;
            }
            else {
                static const bool always_false = false;
                set._valid = &always_false;
            }
        }
    }

    // Make sure we have only one return statement, at the end, to allow copy optimization on return.
    return set;
}


//----------------------------------------------------------------------------
// Find the first child element by name, case-insensitive.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Element::findFirstChild(const UString& name, bool required)
{
    // Loop on all children.
    for (Element* child = firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (name.empty() || child->nameMatch(name)) {
            return child;
        }
    }

    // Child node not found.
    if (required) {
        report().error(u"Child node <%s> not found in <%s>, line %d", name, value(), lineNumber());
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Find the next sibling element by name, case-insensitive.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Element::findNextSibling(const UString& name, bool required)
{
    // Loop on all sibling.
    for (Element* child = nextSiblingElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (name.empty() || child->nameMatch(name)) {
            return child;
        }
    }

    // Sibling node not found.
    if (required) {
        report().error(u"Next node <%s> not found, line %d", name, lineNumber());
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Find all children elements in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

bool ts::xml::Element::getChildren(ElementVector& children, const UString& search_name, size_t min_count, size_t max_count) const
{
    children.clear();

    // Filter invalid parameters.
    if (search_name.empty()) {
        return false;
    }

    // Loop on all children.
    for (const Element* child = firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (child->nameMatch(search_name)) {
            children.push_back(child);
        }
    }

    // Check cardinality.
    if (children.size() >= min_count && children.size() <= max_count) {
        return true;
    }
    else if (max_count == UNLIMITED) {
        report().error(u"<%s>, line %d, contains %d <%s>, at least %d required", name(), lineNumber(), children.size(), search_name, min_count);
        return false;
    }
    else {
        report().error(u"<%s>, line %d, contains %d <%s>, allowed %d to %d", name(), lineNumber(), children.size(), search_name, min_count, max_count);
        return false;
    }
}

//-------------------------------------------------------------------------------
// Check if named child elements are present in an XML element, case-insensitive.
//-------------------------------------------------------------------------------

bool ts::xml::Element::hasChildElement(const UString& search_name) const
{
    bool found = false;

    // Loop on all children.
    for (const Element* child = firstChildElement(); !found && (child != nullptr); child = child->nextSiblingElement()) {
        if (child->nameMatch(search_name)) {
            found = true;
        }
    }
    return found;
}

//----------------------------------------------------------------------------
// Get text in a child of an element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getTextChild(UString& data,
                                    const UString& search_name,
                                    bool trim,
                                    bool required,
                                    const UString& def_value,
                                    size_t min_size,
                                    size_t max_size) const
{
    // Get child node.
    ElementVector child;
    if (!getChildren(child, search_name, required ? 1 : 0, 1)) {
        data.clear();
        return false;
    }

    // Get value in child node.
    if (child.empty()) {
        data = def_value;
        return true;
    }
    else {
        return child[0]->getText(data, trim, min_size, max_size);
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

bool ts::xml::Element::getText(UString& data, const bool trim, size_t min_size, size_t max_size) const
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
    if (len >= min_size && len <= max_size) {
        return true;
    }
    else if (max_size == UNLIMITED) {
        report().error(u"Incorrect text in <%s>, line %d, contains %d characters, at least %d required", name(), lineNumber(), len, min_size);
        return false;
    }
    else {
        report().error(u"Incorrect text in <%s>, line %d, contains %d characters, allowed %d to %d", name(), lineNumber(), len, min_size, max_size);
        return false;
    }
}


//----------------------------------------------------------------------------
// Get text in a child containing hexadecimal data.
//----------------------------------------------------------------------------

bool ts::xml::Element::getHexaTextChild(ByteBlock& data, const UString& search_name, bool required, size_t min_size, size_t max_size) const
{
    // Get child node.
    ElementVector child;
    if (!getChildren(child, search_name, required ? 1 : 0, 1)) {
        data.clear();
        return false;
    }

    // Get value in child node.
    if (child.empty()) {
        data.clear();
        return true;
    }
    else {
        return child[0]->getHexaText(data, min_size, max_size);
    }
}


//----------------------------------------------------------------------------
// Get a text child of an element containing hexadecimal data).
//----------------------------------------------------------------------------

bool ts::xml::Element::getHexaText(ByteBlock& data, size_t min_size, size_t max_size) const
{
    data.clear();

    // Get text children. Ignore errors if no text found, simply empty.
    // Interpret hexa data.
    if (!text().hexaDecode(data)) {
        report().error(u"Invalid hexadecimal content in <%s>, line %d", name(), lineNumber());
        return false;
    }

    // Check value size.
    const size_t len = data.size();
    if (len >= min_size && len <= max_size) {
        return true;
    }
    else if (max_size == UNLIMITED) {
        report().error(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, at least %d required", name(), lineNumber(), len, min_size);
        return false;
    }
    else {
        report().error(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, allowed %d to %d", name(), lineNumber(), len, min_size, max_size);
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

ts::xml::Text* ts::xml::Element::addText(const UString& text, bool only_not_empty)
{
    if (only_not_empty && text.empty()) {
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

ts::xml::Text* ts::xml::Element::addHexaText(const void* data, size_t size, bool only_not_empty)
{
    // Filter incorrect parameters.
    if (data == nullptr) {
        data = "";
        size = 0;
    }

    // Do nothing if empty.
    if (size == 0 && only_not_empty) {
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

ts::xml::Text* ts::xml::Element::addHexaTextChild(const UString& name, const void* data, size_t size, bool only_not_empty)
{
    if (data == nullptr) {
        size = 0;
    }
    return size == 0 && only_not_empty ? nullptr : addElement(name)->addHexaText(data, size);
}

ts::xml::Text* ts::xml::Element::addHexaTextChild(const UString& name, const ByteBlock& data, bool only_not_empty)
{
    return data.empty() && only_not_empty ? nullptr : addElement(name)->addHexaText(data.data(), data.size());
}


//----------------------------------------------------------------------------
// Attribute map management.
//----------------------------------------------------------------------------

ts::xml::Element::AttributeMap::const_iterator ts::xml::Element::findAttribute(const UString& attribute_name) const
{
    AttributeMap::const_iterator it = _attributes.begin();
    while (it != _attributes.end() && !it->second.nameMatch(attribute_name)) {
        ++it;
    }
    return it;
}

ts::xml::Element::AttributeMap::iterator ts::xml::Element::findAttribute(const UString& attribute_name)
{
    AttributeMap::iterator it = _attributes.begin();
    while (it != _attributes.end() && !it->second.nameMatch(attribute_name)) {
        ++it;
    }
    return it;
}

void ts::xml::Element::setAttribute(const UString& name, const UString& value, bool only_if_not_empty)
{
    if (!only_if_not_empty || !value.empty()) {
        const auto it = findAttribute(name);
        if (it == _attributes.end()) {
            _attributes[name] = Attribute(name, value);
        }
        else {
            it->second.setString(value);
        }
    }
}

void ts::xml::Element::deleteAttribute(const UString& name)
{
    const auto it = findAttribute(name);
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
    const auto it = findAttribute(name);
    return it == _attributes.end() ? (_attributes[name] = Attribute(name, u"")) : it->second;
}


//----------------------------------------------------------------------------
// Get an attribute.
//----------------------------------------------------------------------------

const ts::xml::Attribute& ts::xml::Element::attribute(const UString& attribute_name, bool required) const
{
    const auto it = findAttribute(attribute_name);
    if (it != _attributes.end()) {
        // Found the real attribute.
        return it->second;
    }
    if (required) {
        report().error(u"attribute '%s' not found in <%s>, line %d", attribute_name, name(), lineNumber());
    }
    // Return a reference to a static invalid attribute.
    return Attribute::INVALID();
}


//----------------------------------------------------------------------------
// Check if an attribute exists in the element and has the specified value.
//----------------------------------------------------------------------------

bool ts::xml::Element::hasAttribute(const UString& name, const UString& value, bool similar) const
{
    const Attribute& attr(attribute(name));
    std::intmax_t a = 0, b = 0;
    if (!attr.isValid()) {
        // Attribute not present.
        return false;
    }
    else if (!similar) {
        // Strict comparison.
        return value == attr.value();
    }
    else if (value.toInteger(a, UString::DEFAULT_THOUSANDS_SEPARATOR) && attr.value().toInteger(b, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        // The two values are integer, compare the decoded integer values, not their string representation.
        return a == b;
    }
    else {
        // Lousy string comparison.
        return value.similar(attr.value());
    }
}


//----------------------------------------------------------------------------
// Get a string attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getAttribute(UString& value,
                                    const UString& name,
                                    bool required,
                                    const UString& def_value,
                                    size_t min_size,
                                    size_t max_size) const
{
    const Attribute& attr(attribute(name, required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = def_value;
        return !required;
    }
    else {
        // Attribute found, get its value.
        value = attr.value();
        if (value.length() >= min_size && value.length() <= max_size) {
            return true;
        }

        // Incorrect value size.
        if (max_size == UNLIMITED) {
            report().error(u"incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, at least %d required",
                           name, this->name(), attr.lineNumber(), value.length(), min_size);
            return false;
        }
        else {
            report().error(u"oncorrect value for attribute '%s' in <%s>, line %d, contains %d characters, allowed %d to %d",
                           name, this->name(), attr.lineNumber(), value.length(), min_size, max_size);
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Get an optional string attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getOptionalAttribute(std::optional<UString>& value, const UString& name, size_t min_size, size_t max_size) const
{
    // Default: erase value.
    value.reset();
    bool ok = true;

    if (hasAttribute(name)) {
        // Attribute present, value must be correct.
        UString val;
        ok = getAttribute(val, name, true, UString(), min_size, max_size);
        if (ok) {
            value = val;
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Set a Base64-encoded attribute.
//----------------------------------------------------------------------------

void ts::xml::Element::setBase64Attribute(const UString& name, const void* data, size_t size, bool only_not_empty)
{
    // Filter incorrect parameters.
    if (data == nullptr) {
        data = "";
        size = 0;
    }

    // Do nothing if empty.
    if (size > 0 || !only_not_empty) {
        setAttribute(name, Base64::Encoded(data, size));
    }
}


//----------------------------------------------------------------------------
// Get a Base64-encoded attribute.
//----------------------------------------------------------------------------

bool ts::xml::Element::getBase64Attribute(ByteBlock& data, const UString& name, bool required, size_t min_size, size_t max_size) const
{
    data.clear();

    // Get the string version of the attribute.
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }

    // Decode Base64.
    if (!Base64::Decode(data, str)) {
        report().error(u"invalid Base-64 value for attribute '%s' in <%s>, line %d", name, this->name(), lineNumber());
        return false;
    }

    // Check returned size
    if (data.size() >= min_size && data.size() <= max_size) {
        return true;
    }
    else if (max_size == UNLIMITED) {
        report().error(u"invalid value for attribute '%s' in <%s>, line %d, contains %d bytes, at least %d required",
                       name, this->name(), lineNumber(), data.size(), min_size);
        return false;
    }
    else {
        report().error(u"invalid value for attribute '%s' in <%s>, line %d, contains %d bytes, allowed %d to %d",
                       name, this->name(), lineNumber(), data.size(), min_size, max_size);
        return false;
    }
}


//----------------------------------------------------------------------------
// Get a boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getBoolAttribute(bool& value, const UString& name, bool required, bool def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    else if (!required && str.empty()) {
        value = def_value;
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
        report().error(u"'%s' is not a valid boolean value for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
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
// Get a date/time attribute of an XML element.
//----------------------------------------------------------------------------

// Get a date/time attribute of an XML element.
bool ts::xml::Element::getDateTimeAttribute(Time& value, const UString& name, bool required, const Time& def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    // Analyze the time string.
    const bool ok = Attribute::DateTimeFromString(value, str);
    if (!ok) {
        report().error(u"'%s' is not a valid date/time for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD hh:mm:ss\"", str, name, this->name(), lineNumber());
    }
    return ok;
}

// Get a date/time attribute in ISO 8601 representation of an XML element.
bool ts::xml::Element::getISODateTimeAttribute(Time& value, const UString& name, bool required, const Time& def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    // Analyze the time string.
    const bool ok = value.fromISO(str);
    if (!ok) {
        report().error(u"'%s' is not a valid ISO-8601 date/time for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
    }
    return ok;
}

// Get a date/time child element in ISO 8601 representation.
bool ts::xml::Element::getISODateTimeChild(Time& value, const UString& name, bool required , const Time& def_value) const
{
    UString str;
    if (!getTextChild(str, name, true, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    // Analyze the time string.
    const bool ok = value.fromISO(str);
    if (!ok) {
        report().error(u"'%s' is not a valid ISO-8601 date/time for <%s> in <%s>, line %d", str, name, this->name(), lineNumber());
    }
    return ok;
}

// Get an optional date/time attribute of an XML element.
bool ts::xml::Element::getOptionalDateTimeAttribute(std::optional<Time>& value, const UString& name) const
{
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else {
        value.emplace();
        const bool ok = getDateTimeAttribute(value.value(), name, true);
        if (!ok) {
            value.reset();
        }
        return ok;
    }
}


//----------------------------------------------------------------------------
// Get a date attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getDateAttribute(Time& value, const UString& name, bool required, const Time& def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    // Analyze the time string.
    const bool ok = Attribute::DateFromString(value, str);
    if (!ok) {
        report().error(u"'%s' is not a valid date for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD\"", str, name, this->name(), lineNumber());
    }
    return ok;
}

bool ts::xml::Element::getOptionalDateAttribute(std::optional<Time>& value, const UString& name) const
{
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else {
        value.emplace();
        const bool ok = getDateAttribute(value.value(), name, true);
        if (!ok) {
            value.reset();
        }
        return ok;
    }
}


//----------------------------------------------------------------------------
// Get an IPv4/v6 or MAC address attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::xml::Element::getIPAttribute(IPAddress& value, const UString& name, bool required, const IPAddress& def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    const bool ok = value.resolve(str, report());
    if (!ok) {
        report().error(u"'%s' is not a valid IP address for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
    }
    return ok;
}

bool ts::xml::Element::getIPChild(IPAddress& value, const UString& name, bool required, const IPAddress& def_value) const
{
    UString str;
    if (!getTextChild(str, name, true, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    const bool ok = value.resolve(str, report());
    if (!ok) {
        report().error(u"'%s' is not a valid IP address in <%s><%s>, line %d", str, this->name(), name, lineNumber());
    }
    return ok;
}

bool ts::xml::Element::getMACAttribute(MACAddress& value, const UString& name, bool required, const MACAddress& def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = def_value;
        return true;
    }

    const bool ok = value.resolve(str, report());
    if (!ok) {
        report().error(u"'%s' is not a valid MAC address for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
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
    using NameMap = std::multimap<size_t, UString>;
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

bool ts::xml::Element::merge(Element* other, MergeAttributes attr_options)
{
    // Ignore null or self merge.
    if (other == nullptr || other == this) {
        return true;
    }

    // Check that the elements have identical tags.
    if (!nameMatch(other)) {
        report().error(u"Cannot merge XML element <%s>, line %d, with <%s>, line %d", name(), lineNumber(), other->name(), other->lineNumber());
        return false;
    }

    // Merge attributes.
    if (attr_options != MergeAttributes::NONE) {
        for (const auto& attr : other->_attributes) {
            if (attr_options == MergeAttributes::REPLACE || !hasAttribute(attr.second.name())) {
                setAttribute(attr.second.name(), attr.second.value());
            }
        }
    }

    // Remove elements one by one from the node to merge.
    xml::Element* elem = nullptr;
    while ((elem = other->firstChildElement()) != nullptr) {
        // We need to merge its content with an element of the same name in the main.
        xml::Element* main = findFirstChild(elem->name());
        if (main == nullptr) {
            // The tag did not exist in the main element, simply move is here.
            elem->reparent(this);
        }
        else {
            // Move all content into the main topic.
            main->merge(elem, attr_options);
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

void ts::xml::Element::print(TextFormatter& output, bool keep_node_open) const
{
    // Output element name.
    output << "<" << name();

    // Get all attributes names, by modification order.
    UStringList names;
    getAttributesNamesInModificationOrder(names);

    // Loop on all attributes.
    for (const auto& atname : names) {
        const Attribute& attr(attribute(atname, true));
        output << " " << attr.name() << "=" << attr.formattedValue(tweaks());
    }

    // Close the tag and return if nothing else to output.
    if (!hasChildren() && !keep_node_open) {
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
    if (!sticky || keep_node_open) {
        output << ts::endl;
    }
    if (!keep_node_open) {
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
    UString node_name;
    parser.skipWhiteSpace();
    if (!parser.parseXMLName(node_name)) {
        report().error(u"line %d: parsing error, tag name expected", parser.lineNumber());
        return false;
    }

    // The "value" of an element is its tag name.
    setValue(node_name);

    // Read the list of attributes.
    bool ok = true;
    while (ok) {
        UString attr_name;
        UString attr_value;
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
        else if (parser.parseXMLName(attr_name)) {
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
            ok = ok && parser.parseText(attr_value, quote, true, true);

            // Store the attribute.
            if (!ok) {
                report().error(u"line %d: error parsing attribute '%s' in tag <%s>", line, attr_name, value());
            }
            else if (hasAttribute(attr_name)) {
                report().error(u"line %d: duplicate attribute '%s' in tag <%s>", line, attr_name, value());
                ok = false;
            }
            else {
                _attributes[attr_name] = Attribute(attr_name, attr_value, line);
                // When attribute is xml:space="preserve", spaces shall be preserved in that hierarchy.
                if (attr_name == u"xml:space" && attr_value == u"preserve") {
                    setPreserveSpace(true);
                }
            }
        }
        else {
            report().error(u"line %d: parsing error, tag <%s>", lineNumber(), value());
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
        report().error(u"line %d: parsing error, expected </%s> to match <%s> at line %d", parser.lineNumber(), value(), value(), lineNumber());
    }

    return ok;
}
