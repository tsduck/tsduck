//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsKeyTable.h"
#include "tsxmlModelDocument.h"
#include "tsxmlElement.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Check the presence of a key in the table.
//----------------------------------------------------------------------------

bool ts::KeyTable::hasKey(const ByteBlock& id) const
{
    return Contains(_keys, id);
}

bool ts::KeyTable::hasKey(const UString& id) const
{
    ByteBlock bid;
    return id.hexaDecode(bid) && hasKey(bid);
}


//----------------------------------------------------------------------------
// Store a key in the table.
//----------------------------------------------------------------------------

bool ts::KeyTable::storeKey(const ByteBlock& id, const ByteBlock& value, bool replace)
{
    if (!replace && hasKey(id)) {
        return false; // key already exists
    }
    else {
        _keys[id] = value;
        return true;
    }
}

bool ts::KeyTable::storeKey(const UString& id, const UString& value, bool replace)
{
    ByteBlock bid;
    ByteBlock bvalue;
    return id.hexaDecode(bid) && value.hexaDecode(bvalue) && storeKey(bid, bvalue, replace);
}


//----------------------------------------------------------------------------
// Get the value of a key from the table.
//----------------------------------------------------------------------------

bool ts::KeyTable::getKey(const ByteBlock& id, ByteBlock& value) const
{
    const auto it = _keys.find(id);
    if (it == _keys.end()) {
        value.clear();
        return false;
    }
    else {
        value = it->second;
        return true;
    }
}

bool ts::KeyTable::getKey(const UString& id, ByteBlock& value) const
{
    ByteBlock bid;
    return id.hexaDecode(bid) && getKey(bid, value);
}


//----------------------------------------------------------------------------
// Retrieve a key in the table and initialize a block cipher engine.
//----------------------------------------------------------------------------

bool ts::KeyTable::setKey(BlockCipher& cipher, const ByteBlock& id, size_t rounds) const
{
    ByteBlock value;
    return getKey(id, value) && cipher.setKey(value.data(), value.size(), rounds);
}

bool ts::KeyTable::setKey(BlockCipher& cipher, const UString& id, size_t rounds) const
{
    ByteBlock bid;
    return id.hexaDecode(bid) && setKey(cipher, bid, rounds);
}


//----------------------------------------------------------------------------
// Load all keys from an XML string and add them in the key table.
//----------------------------------------------------------------------------

bool ts::KeyTable::loadXML(Report& report, const UString& text, bool replace, size_t id_size, size_t value_size)
{
    xml::Document doc(report);
    return doc.parse(text) && parseXML(doc, replace, id_size, value_size);
}


//----------------------------------------------------------------------------
// Load all keys from an XML file and add them in the key table.
//----------------------------------------------------------------------------

bool ts::KeyTable::loadFile(Report& report, const UString& filename, bool replace, size_t id_size, size_t value_size)
{
    // Do not search in TSDuck directory, use plain file specification.
    xml::Document doc(report);
    report.debug(u"loading %s", {filename});
    return doc.load(filename, false) && parseXML(doc, replace, id_size, value_size);
}


//----------------------------------------------------------------------------
// Common code for loadFile() and loadXML().
//----------------------------------------------------------------------------

bool ts::KeyTable::parseXML(xml::Document& doc, bool replace, size_t id_size, size_t value_size)
{
    // Load the XML model. Search it in TSDuck directory.
    xml::ModelDocument model(doc.report());
    if (!model.load(u"tsduck.keytable.model.xml", true)) {
        doc.report().error(u"Model for TSDuck key table XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!model.validate(doc)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();

    // Get all <key> elements in the document.
    xml::ElementVector keys;
    if (root == nullptr || !root->getChildren(keys, u"key")) {
        doc.report().error(u"no <key> found in XML key file");
        return false;
    }

    // Analyze all keys.
    doc.report().debug(u"loaded %d key records", {keys.size()});
    bool success = true;
    for (size_t i = 0; i < keys.size(); ++i) {
        UString id, value;
        ByteBlock bid, bvalue;
        const xml::Element* const k = keys[i];
        if (!k->getAttribute(id, u"id", true) || !k->getAttribute(value, u"value", true)) {
            // Error message already reported.
            success = false;
        }
        else if (!id.hexaDecode(bid) || (id_size != 0 && bid.size() != id_size)) {
            doc.report().error(u"invalid id in <%s> at line %d", {k->name(), k->lineNumber()});
            success = false;
        }
        else if (!value.hexaDecode(bvalue) || (value_size != 0 && bvalue.size() != value_size)) {
            doc.report().error(u"invalid value in <%s> at line %d", {k->name(), k->lineNumber()});
        }
        else if (replace || !hasKey(bid)) {
            _keys[bid] = bvalue;
        }
    }
    return success;
}
