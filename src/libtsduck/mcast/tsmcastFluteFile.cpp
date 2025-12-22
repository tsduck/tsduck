//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFluteFile.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::mcast::FluteFile::FluteFile(const FluteSessionId& sid,
                         uint64_t              toi,
                         const UString&        name,
                         const UString&        type,
                         const ByteBlockPtr&   content) :
    _sid(sid),
    _toi(toi),
    _name(name),
    _type(type),
    _full_type(type),
    _content(content != nullptr ? content : std::make_shared<ByteBlock>())
{
    // Remove qualification such as "charset=utf-8" in type.
    const size_t sc = _type.find(u";");
    if (sc < _type.length()) {
        _type.resize(sc);
    }
}

ts::mcast::FluteFile::~FluteFile()
{
}


//----------------------------------------------------------------------------
// Get a character string version of the file, if it is a text file.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FluteFile::toText() const
{
    return UString::FromUTF8(reinterpret_cast<const char*>(_content->data()), _content->size());
}


//----------------------------------------------------------------------------
// Get an indented XML character string version of the file.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FluteFile::toXML() const
{
    UString text(toText());
    xml::Document doc;
    if (doc.parse(text)) {
        text = doc.toString();
    }
    text.trim(false, true);
    return text;
}


//----------------------------------------------------------------------------
// Parse the document using XML format.
//----------------------------------------------------------------------------

bool ts::mcast::FluteFile::parseXML(xml::Document& xml_doc, const UChar* expected_root, bool ignore_namespace)
{
    // Parse the XML document.
    _valid = xml_doc.parse(toText());
    xml_doc.setIignoreNamespace(ignore_namespace);

    if (_valid && expected_root != nullptr && *expected_root != CHAR_NULL) {
        const xml::Element* root = xml_doc.rootElement();
        if (root == nullptr) {
            _valid = false;
            xml_doc.report().error(u"XML root element not found in %s, %s", _name, _sid);
        }
        else if (!root->nameMatch(expected_root)) {
            _valid = false;
            xml_doc.report().error(u"invalid XML root element <%s>, expected <%s>, in %s, %s", root->name(), expected_root, _name, _sid);
        }
    }

    return _valid;
}
