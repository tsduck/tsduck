//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsxmlJSONConverter.h"
#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsjsonArray.h"
#include "tsjsonNull.h"
#include "tsjsonNumber.h"
#include "tsjsonString.h"
TSDUCK_SOURCE;


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

ts::json::ValuePtr ts::xml::JSONConverter::convert(const Document& source, bool force_root) const
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
            return convertElement(modelRoot, docRoot, tweaks());
        }
        else {
            // Return a JSON array of all top-level elements in the root.
            return convertChildren(modelRoot, docRoot, tweaks());
        }
    }
}


//----------------------------------------------------------------------------
// Convert an XML tree of elements.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::xml::JSONConverter::convertElement(const Element* model, const Element* source, const Tweaks& xml_tweaks) const
{
    // Build the JSON object for the node.
    json::ValuePtr jobj(new json::Object());
    CheckNonNull(jobj.pointer());
    jobj->add(u"#name", source->name());

    // Get all attributes of the XML element.
    std::map<UString,UString> attributes;
    source->getAttributes(attributes);

    // Add attributes in the JSON object.
    for (auto it = attributes.begin(); it != attributes.end(); ++it) {

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
            model->getAttribute(description, it->first, false);
            description.trim(true, false, false);
            intModel = description.startWith(u"uint", CASE_INSENSITIVE) || description.startWith(u"int", CASE_INSENSITIVE);
            boolModel = description.startWith(u"bool", CASE_INSENSITIVE);
        }

        // Try to convert as an integer or boolean if defined as such by the model.
        if (intModel) {
            // Should be an integer according to the model.
            if (it->second.toInteger(intValue, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
                jvalue = new json::Number(intValue);
            }
            else {
                source->report().warning(u"attribute '%s' in <%s> line %d is '%s' but should be an integer", {it->first, source->name(), source->lineNumber(), it->second});
            }
        }
        else if (boolModel) {
            // Should be a boolean according to the model.
            if (it->second.toBool(boolValue)) {
                jvalue = json::Bool(boolValue);
            }
            else {
                source->report().warning(u"attribute '%s' in <%s> line %d is '%s' but should be a boolean", {it->first, source->name(), source->lineNumber(), it->second});
            }
        }

        // Try to enforce integer of boolean value if specified on command line.
        if (jvalue.isNull() && xml_tweaks.x2jEnforceInteger && !intModel && it->second.toInteger(intValue, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
            jvalue = new json::Number(intValue);
        }
        if (jvalue.isNull() && xml_tweaks.x2jEnforceBoolean && !boolModel && it->second.toBool(boolValue)) {
            jvalue = json::Bool(boolValue);
        }

        // Use a string value by default.
        if (jvalue.isNull()) {
            jvalue = new json::String(it->second);
        }

        // Add the attribute in the JSON object.
        jobj->add(it->first, jvalue);
    }

    // Process the list of children, if any.
    if (source->hasChildren()) {
        jobj->add(u"#nodes", convertChildren(model, source, xml_tweaks));
    }

    return jobj;
}


//----------------------------------------------------------------------------
// Convert all children of an element as a JSON array.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::xml::JSONConverter::convertChildren(const Element* model, const Element* parent, const Tweaks& xml_tweaks) const
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
            jchildren->set(convertElement(findModelElement(model, elem->name()), elem, xml_tweaks));
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
