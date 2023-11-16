//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsxmlJSONConverter.h"
#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsjsonArray.h"
#include "tsjsonNull.h"
#include "tsjsonNumber.h"
#include "tsjsonObject.h"
#include "tsjsonString.h"
#include "tsFatal.h"

const ts::UString ts::xml::JSONConverter::HashName(u"#name");
const ts::UString ts::xml::JSONConverter::HashNodes(u"#nodes");
const ts::UString ts::xml::JSONConverter::HashUnnamed(u"_unnamed");


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::JSONConverter::JSONConverter(Report& report) :
    ModelDocument(report)
{
}

ts::xml::JSONConverter::~JSONConverter()
{
}


//----------------------------------------------------------------------------
// Convert an XML document into a JSON object.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::xml::JSONConverter::convertToJSON(const Document& source, bool force_root) const
{
    const xml::Element* docRoot = source.rootElement();

    if (docRoot == nullptr) {
        report().error(u"invalid XML document, no root element");
        return json::ValuePtr(new json::Null());
    }
    else {
        // Ignore the model if the model root has a different name from the source root.
        const Element* modelRoot = rootElement();
        if (modelRoot != nullptr && !modelRoot->name().similar(docRoot->name())) {
            modelRoot = nullptr;
        }

        // Convert the source. Use no model if not the same as source.
        if (tweaks().x2jIncludeRoot || force_root) {
            // Return a JSON object containing the root.
            return convertElementToJSON(modelRoot, docRoot, tweaks());
        }
        else {
            // Return a JSON array of all top-level elements in the root.
            return convertChildrenToJSON(modelRoot, docRoot, tweaks());
        }
    }
}


//----------------------------------------------------------------------------
// Convert an XML tree of elements.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::xml::JSONConverter::convertElementToJSON(const Element* model, const Element* source, const Tweaks& xml_tweaks) const
{
    // Build the JSON object for the node.
    json::ValuePtr jobj(new json::Object());
    CheckNonNull(jobj.pointer());
    jobj->add(HashName, source->name());

    // Get all attributes of the XML element.
    std::map<UString,UString> attributes;
    source->getAttributes(attributes);

    // Add attributes in the JSON object.
    for (const auto& it : attributes) {

        // JSON value of the attribute.
        json::ValuePtr jvalue;
        int64_t intValue = 0;
        bool boolValue = false;

        // Get description of this attribute in the model.
        UString description;
        bool intModel = false;
        bool boolModel = false;
        if (model != nullptr) {
            // Get description, empty string without error if not found.
            model->getAttribute(description, it.first, false);
            description.trim(true, false, false);
            intModel = description.startWith(u"uint", CASE_INSENSITIVE) || description.startWith(u"int", CASE_INSENSITIVE);
            boolModel = description.startWith(u"bool", CASE_INSENSITIVE);
        }

        // Try to convert as an integer or boolean if defined as such by the model.
        if (intModel) {
            // Should be an integer according to the model.
            if (it.second.toInteger(intValue, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
                if (intValue < -0xFFFFFFFFLL) {
                    // This is a "very negative" value. This is typically a large unsigned hexadecimal value
                    // which will not be handled correctly when reading back the JSON file. We cannot use
                    // hexadecimal literals in JSON (new in JSON 5), so we leave it as a string.
                    jvalue = new json::String(it.second);
                }
                else {
                    // Acceptable integer.
                    jvalue = new json::Number(intValue);
                }
            }
            else {
                source->report().warning(u"attribute '%s' in <%s> line %d is '%s' but should be an integer", {it.first, source->name(), source->lineNumber(), it.second});
            }
        }
        else if (boolModel) {
            // Should be a boolean according to the model.
            if (it.second.toBool(boolValue)) {
                jvalue = json::Bool(boolValue);
            }
            else {
                source->report().warning(u"attribute '%s' in <%s> line %d is '%s' but should be a boolean", {it.first, source->name(), source->lineNumber(), it.second});
            }
        }

        // Try to enforce integer of boolean value if specified on command line.
        if (jvalue.isNull() && xml_tweaks.x2jEnforceInteger && !intModel && it.second.toInteger(intValue, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
            jvalue = new json::Number(intValue);
        }
        if (jvalue.isNull() && xml_tweaks.x2jEnforceBoolean && !boolModel && it.second.toBool(boolValue)) {
            jvalue = json::Bool(boolValue);
        }

        // Use a string value by default.
        if (jvalue.isNull()) {
            jvalue = new json::String(it.second);
        }

        // Add the attribute in the JSON object.
        jobj->add(it.first, jvalue);
    }

    // Process the list of children, if any.
    if (source->hasChildren()) {
        jobj->add(HashNodes, convertChildrenToJSON(model, source, xml_tweaks));
    }

    return jobj;
}


//----------------------------------------------------------------------------
// Convert all children of an element as a JSON array.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::xml::JSONConverter::convertChildrenToJSON(const Element* model, const Element* parent, const Tweaks& xml_tweaks) const
{
    // All JSON children are placed in an array.
    json::ValuePtr jchildren(new json::Array());
    CheckNonNull(jchildren.pointer());

    // Content of the text children in the model.
    UString textModel;
    bool getTextModel = model != nullptr;
    bool hexaModel = false;

    // Loop on all children nodes.
    bool lastNode = false;
    for (const Node* child = parent->firstChild(); child != nullptr && !lastNode; child = child->nextSibling()) {
        lastNode = child == parent->lastChild();

        // Interpret the child either as an Element or a Text node.
        // Other types of nodes are ignored.
        const Element* elem = dynamic_cast<const Element*>(child);
        const Text* text = dynamic_cast<const Text*>(child);

        if (elem != nullptr) {
            // Convert an element. Add a JSON child object in the array of JSON children.
            jchildren->set(convertElementToJSON(findModelElement(model, elem->name()), elem, xml_tweaks));
        }
        else if (text != nullptr) {
            // Convert a text.
            UString content(text->value());
            // Get the model description once only.
            if (getTextModel) {
                getTextModel = false;
                model->getText(textModel, true);
                hexaModel = textModel.startWith(u"hexa", CASE_INSENSITIVE);
            }
            // Trim the text content according to model and command line options.
            content.trim(hexaModel || xml_tweaks.x2jTrimText, hexaModel || xml_tweaks.x2jTrimText, hexaModel || xml_tweaks.x2jCollapseText);
            // Add a JSON string for the text node in the array of JSON children.
            jchildren->set(content);
        }
    }
    return jchildren;
}


//----------------------------------------------------------------------------
// Build a valid XML element name from a JSON string.
//----------------------------------------------------------------------------

ts::UString ts::xml::JSONConverter::ToElementName(const UString& str)
{
    UString result;
    result.reserve(3 * str.length());
    for (size_t i = 0; i < str.length(); ++i) {
        const UChar c = str[i];
        if (IsAlpha(c) || c == u'_') {
            result.push_back(c);
        }
        else if (i > 0 && (IsDigit(c) || c == u'-' || c == u'.')) {
            result.push_back(c);
        }
        else {
            result.format(u"_%02X", {int(c)});
        }
    }
    return result;
}


//----------------------------------------------------------------------------
// Get the name of a JSON object for reverse conversion.
//----------------------------------------------------------------------------

ts::UString ts::xml::JSONConverter::ElementNameOf(const json::Value& obj, const UString& default_name)
{
    const json::Value& jname(obj.value(HashName));
    if (jname.isString() && jname.size() > 0) {
        return ToElementName(jname.toString());
    }
    else {
        return default_name.empty() ? HashUnnamed : default_name;
    }
}


//----------------------------------------------------------------------------
// Convert a JSON object into an XML document.
//----------------------------------------------------------------------------

bool ts::xml::JSONConverter::convertToXML(const json::Value& source, Document& destination, bool auto_validate) const
{
    // Clear the destination document.
    destination.clear();

    // Create the XML root of the destination using the name in the root JSON object (or the model root).
    const Element* const modelRoot = rootElement();
    Element* const destRoot = destination.initialize(ElementNameOf(source, modelRoot != nullptr ? modelRoot->name() : UString()));

    // Now convert the structure
    if (source.isObject()) {
        // The JSON root is an object => XML root element
        convertObjectToXML(destRoot, source);
    }
    else if (source.isArray()) {
        // The JSON root is an object => children of the XML root element
        convertArrayToXML(destRoot, source);
    }
    else {
        // Other forms of root are unexpected, use a text node with the value.
        destRoot->addText(source.toString(), true);
    }

    // Finally, validate the converted document, if correctly converted.
    return !auto_validate || validate(destination);
}


//----------------------------------------------------------------------------
// Convert a JSON object into an XML element.
//----------------------------------------------------------------------------

void ts::xml::JSONConverter::convertObjectToXML(Element* element, const json::Value& object) const
{
    assert(object.isObject());

    // Get the list of all attribute names in the object.
    UStringList names;
    object.getNames(names);

    for (const auto& it : names) {
        const json::Value& child(object.value(it));
        if (it.similar(HashName)) {
            // The "#name" was the name of the element, already used.
        }
        else if (it.similar(HashNodes)) {
            // The value must be an array of child elements.
            convertArrayToXML(element, child);
        }
        else if (child.isObject()) {
            // Not expected in a reverse conversion, create an XML element from it.
            Element* e = element->addElement(ElementNameOf(child));
            convertObjectToXML(e, child);
        }
        else if (child.isArray()) {
            // Not expected in a reverse conversion, create an XML element from each array element.
            Element* e = element->addElement(ElementNameOf(child));
            convertArrayToXML(e, child);
        }
        else if (!child.isNull()) {
            // An attribute of the parent element.
            element->setAttribute(ToElementName(it), child.toString());
        }
    }
}


//----------------------------------------------------------------------------
// Convert a JSON array into an children of an XML element.
//----------------------------------------------------------------------------

void ts::xml::JSONConverter::convertArrayToXML(Element* parent, const json::Value& array) const
{
    assert(array.isArray());

    // Each element in the array is a direct child of the parent.
    for (size_t i = 0; i < array.size(); ++i) {
        const json::Value& child(array.at(i));
        if (child.isObject()) {
            Element* e = parent->addElement(ElementNameOf(child));
            convertObjectToXML(e, child);
        }
        else if (child.isArray()) {
            // Not expected in a reverse conversion, create a direct child XML element from each array element.
            convertArrayToXML(parent, child);
        }
        else if (!child.isNull()) {
            // A text node.
            parent->addText(child.toString());
        }
    }
}
