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
//
//  XML files containing PSI/SI tables.
//
//----------------------------------------------------------------------------

#include "tsXMLTables.h"
#include "tsAbstractTable.h"
#include "tsAbstractDescriptor.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define XML_GENERIC_DESCRIPTOR   u"generic_descriptor"
#define XML_GENERIC_SHORT_TABLE  u"generic_short_table"
#define XML_GENERIC_LONG_TABLE   u"generic_long_table"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::XMLTables::XMLTables() :
    _tables()
{
}


//----------------------------------------------------------------------------
// Add a table in the file.
//----------------------------------------------------------------------------

void ts::XMLTables::add(const AbstractTablePtr& table, const DVBCharset* charset)
{
    if (!table.isNull() && table->isValid()) {
        BinaryTablePtr bin(new BinaryTable);
        table->serialize(*bin, charset);
        if (bin->isValid()) {
            add(bin);
        }
    }
}


//----------------------------------------------------------------------------
// Load / parse an XML file.
//----------------------------------------------------------------------------

bool ts::XMLTables::loadXML(const UString& file_name, Report& report, const DVBCharset* charset)
{
    clear();
    xml::Document doc(report);
    return doc.load(file_name, false) && parseDocument(doc, charset);
}

bool ts::XMLTables::parseXML(const UString& xml_content, Report& report, const DVBCharset* charset)
{
    clear();
    xml::Document doc(report);
    return doc.parse(xml_content) && parseDocument(doc, charset);
}

bool ts::XMLTables::parseDocument(const xml::Document& doc, const DVBCharset* charset)
{
    // Load the XML model for TSDuck files. Search it in TSDuck directory.
    xml::Document model(doc.report());
    if (!model.load(u"tsduck.xml", true)) {
        doc.report().error(u"Model for TSDuck XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!doc.validate(model)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();
    bool success = true;

    // Analyze all tables in the document.
    for (const xml::Element* node = root == 0 ? 0 : root->firstChildElement(); node != 0; node = node->nextSiblingElement()) {

        BinaryTablePtr bin;
        const UString name(node->name());

        // Get the table factory for that kind of XML tag.
        const TablesFactory::TableFactory fac = TablesFactory::Instance()->getTableFactory(name);
        if (fac != 0) {
            // Create a table instance of the right type.
            AbstractTablePtr table = fac();
            if (!table.isNull()) {
                table->fromXML(node);
            }
            if (!table.isNull() && table->isValid()) {
                // Serialize the table.
                bin = new BinaryTable;
                table->serialize(*bin, charset);
            }
        }
        else {
            // No known factory, add a generic table.
            bin = FromGenericTableXML(node);
        }

        // Insert created table or report error.
        if (!bin.isNull() && bin->isValid()) {
            _tables.push_back(bin);
        }
        else {
            doc.report().error(u"Error in table <%s> at line %d", {name, node->lineNumber()});
            success = false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Create XML file or text.
//----------------------------------------------------------------------------

bool ts::XMLTables::saveXML(const UString& file_name, Report& report, const DVBCharset* charset) const
{
    xml::Document doc(report);
    return generateDocument(doc, charset) && doc.save(file_name);
}

ts::UString ts::XMLTables::toText(Report& report, const DVBCharset* charset) const
{
    xml::Document doc(report);
    return generateDocument(doc, charset) ? doc.toString() : UString();
}


//----------------------------------------------------------------------------
// Generate an XML document.
//----------------------------------------------------------------------------

bool ts::XMLTables::generateDocument(xml::Document& doc, const DVBCharset* charset) const
{
    // Initialize the document structure.
    xml::Element* root = doc.initialize(u"tsduck");
    if (root == 0) {
        return false;
    }

    // Format all tables.
    for (BinaryTablePtrVector::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
        const BinaryTablePtr& table(*it);
        if (!table.isNull()) {
            ToXML(root, *table, charset);
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// This method converts a table to the appropriate XML tree.
//----------------------------------------------------------------------------

ts::xml::Element* ts::XMLTables::ToXML(xml::Element* parent, const BinaryTable& table, const DVBCharset* charset)
{
    // Filter invalid tables.
    if (!table.isValid()) {
        return 0;
    }

    // The XML node we will generate.
    xml::Element* node = 0;

    // Do we know how to deserialize this table?
    TablesFactory::TableFactory fac = TablesFactory::Instance()->getTableFactory(table.tableId());
    if (fac != 0) {
        // We know how to deserialize this table.
        AbstractTablePtr tp = fac();
        if (!tp.isNull()) {
            // Deserialize from binary to object.
            tp->deserialize(table, charset);
            if (tp->isValid()) {
                // Serialize from object to XML.
                node = tp->toXML(parent);
            }
        }
    }

    // If we could not generate a typed node, generate a generic one.
    if (node == 0) {
        node = ToGenericTable(parent, table);
    }

    return node;
}


//----------------------------------------------------------------------------
// This method converts a descriptor to the appropriate XML tree.
//----------------------------------------------------------------------------

ts::xml::Element* ts::XMLTables::ToXML(xml::Element* parent, const Descriptor& desc, PDS pds, const DVBCharset* charset)
{
    // Filter invalid descriptors.
    if (!desc.isValid()) {
        return 0;
    }

    // The XML node we will generate.
    xml::Element* node = 0;

    // Do we know how to deserialize this descriptor?
    TablesFactory::DescriptorFactory fac = TablesFactory::Instance()->getDescriptorFactory(desc.edid(pds));
    if (fac != 0) {
        // We know how to deserialize it.
        AbstractDescriptorPtr dp = fac();
        if (!dp.isNull()) {
            // Deserialize from binary to object.
            dp->deserialize(desc, charset);
            if (dp->isValid()) {
                // Serialize from object to XML.
                node = dp->toXML(parent);
            }
        }
    }

    // If we could not generate a typed node, generate a generic one.
    if (node == 0) {
        node = ToGenericDescriptor(parent, desc);
    }

    return node;
}


//----------------------------------------------------------------------------
// This method converts a list of descriptors to XML.
//----------------------------------------------------------------------------

bool ts::XMLTables::ToXML(xml::Element* parent, const DescriptorList& list, const DVBCharset* charset)
{
    bool success = true;
    for (size_t index = 0; index < list.count(); ++index) {
        const DescriptorPtr desc(list[index]);
        if (desc.isNull() || ToXML(parent, *desc, list.privateDataSpecifier(index), charset) == 0) {
            success = false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// This method converts a generic table to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::XMLTables::ToGenericTable(xml::Element* parent, const BinaryTable& table)
{
    // Filter invalid tables.
    if (!table.isValid() || table.sectionCount() == 0) {
        return 0;
    }
    SectionPtr section(table.sectionAt(0));
    if (section.isNull()) {
        return 0;
    }

    if (table.isShortSection()) {
        // Create a short section node.
        xml::Element* root = parent->addElement(u"generic_short_table");
        root->setIntAttribute(u"table_id", section->tableId(), true);
        root->setBoolAttribute(u"private", section->isPrivateSection());
        root->addHexaText(section->payload(), section->payloadSize());
        return root;
    }
    else {
        // Create a table with long sections.
        xml::Element* root = parent->addElement(u"generic_long_table");
        root->setIntAttribute(u"table_id", table.tableId(), true);
        root->setIntAttribute(u"table_id_ext", table.tableIdExtension(), true);
        root->setIntAttribute(u"version", table.version());
        root->setBoolAttribute(u"current", section->isCurrent());
        root->setBoolAttribute(u"private", section->isPrivateSection());

        // Add each section in binary format.
        for (size_t index = 0; index < table.sectionCount(); ++index) {
            section = table.sectionAt(index);
            if (!section.isNull() && section->isValid()) {
                root->addElement(u"section")->addHexaText(section->payload(), section->payloadSize());
            }
        }
        return root;
    }
}


//----------------------------------------------------------------------------
// This method converts a generic descriptor to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::XMLTables::ToGenericDescriptor(xml::Element* parent, const Descriptor& desc)
{
    // Filter invalid descriptor.
    if (!desc.isValid()) {
        return 0;
    }

    // Create the XML node.
    xml::Element* root = parent->addElement(XML_GENERIC_DESCRIPTOR);
    root->setIntAttribute(u"tag", desc.tag(), true);
    root->addHexaText(desc.payload(), desc.payloadSize());
    return root;
}


//----------------------------------------------------------------------------
// This method decodes an XML list of descriptors.
//----------------------------------------------------------------------------

bool ts::XMLTables::FromDescriptorListXML(DescriptorList& list, xml::ElementVector& others, const xml::Element* parent, const UString& allowedOthers, const DVBCharset* charset)
{
    UStringList allowed;
    allowedOthers.split(allowed);
    return FromDescriptorListXML(list, others, parent, allowed, charset);
}

bool ts::XMLTables::FromDescriptorListXML(DescriptorList& list, const xml::Element* parent)
{
    xml::ElementVector others;
    return FromDescriptorListXML(list, others, parent, UStringList());
}

bool ts::XMLTables::FromDescriptorListXML(DescriptorList& list, xml::ElementVector& others, const xml::Element* parent, const UStringList& allowedOthers, const DVBCharset* charset)
{
    bool success = true;
    list.clear();
    others.clear();

    // Analyze all children nodes.
    for (const xml::Element* node = parent == 0 ? 0 : parent->firstChildElement(); node != 0; node = node->nextSiblingElement()) {

        DescriptorPtr bin;
        const UString name(node->name());
        bool isDescriptor = false;

        // Get the descriptor factory for that kind of XML tag.
        const TablesFactory::DescriptorFactory fac = TablesFactory::Instance()->getDescriptorFactory(name);
        if (fac != 0) {
            isDescriptor = true;
            // Create a descriptor instance of the right type.
            AbstractDescriptorPtr desc = fac();
            if (!desc.isNull()) {
                desc->fromXML(node);
            }
            if (!desc.isNull() && desc->isValid()) {
                // Serialize the descriptor.
                bin = new Descriptor;
                CheckNonNull(bin.pointer());
                desc->serialize(*bin, charset);
            }
        }
        else if (name.similar(XML_GENERIC_DESCRIPTOR)) {
            isDescriptor = true;
            // Add a generic descriptor.
            bin = FromGenericDescriptorXML(node);
        }

        if (isDescriptor) {
            // The tag is a valid descriptor name.
            if (!bin.isNull() && bin->isValid()) {
                list.add(bin);
            }
            else {
                parent->report().error(u"Error in descriptor <%s> at line %d", {name, node->lineNumber()});
                success = false;
            }
        }
        else {
            // The tag is not a descriptor name, check if this is one of the allowed node.
            if (name.containSimilar(allowedOthers)) {
                others.push_back(node);
            }
            else {
                parent->report().error(u"Illegal <%s> at line %d", {name, node->lineNumber()});
                success = false;
            }
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// This method decodes a <generic_short_table> or <generic_long_table>.
//----------------------------------------------------------------------------

ts::BinaryTablePtr ts::XMLTables::FromGenericTableXML(const xml::Element* elem)
{
    // Silently ignore invalid parameters.
    if (elem == 0) {
        return BinaryTablePtr();
    }

    // There are two possible forms of generic tables.
    const UString name(elem->name());
    if (name.similar(XML_GENERIC_SHORT_TABLE)) {

        TID tid = 0xFF;
        bool priv = true;
        ByteBlock payload;
        bool ok =
            elem->getIntAttribute<TID>(tid, u"table_id", true, 0xFF, 0x00, 0xFF) &&
            elem->getBoolAttribute(priv, u"private", false, true) &&
            elem->getHexaText(payload, 0, MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE);

        if (ok) {
            BinaryTablePtr table(new BinaryTable);
            table->addSection(SectionPtr(new Section(tid, priv, payload.data(), payload.size())));
            if (table->isValid()) {
                return table;
            }
        }
    }
    else if (name.similar(XML_GENERIC_LONG_TABLE)) {

        TID tid = 0xFF;
        uint16_t tidExt = 0xFFFF;
        uint8_t version = 0;
        bool priv = true;
        bool current = true;
        xml::ElementVector sectionNodes;
        bool ok =
            elem->getIntAttribute<TID>(tid, u"table_id", true, 0xFF, 0x00, 0xFF) &&
            elem->getIntAttribute<uint16_t>(tidExt, u"table_id_ext", false, 0xFFFF, 0x0000, 0xFFFF) &&
            elem->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
            elem->getBoolAttribute(current, u"current", false, true) &&
            elem->getBoolAttribute(priv, u"private", false, true) &&
            elem->getChildren(sectionNodes, u"section", 1, 256);

        if (ok) {
            BinaryTablePtr table(new BinaryTable);
            for (size_t index = 0; ok && index < sectionNodes.size(); ++index) {
                assert(sectionNodes[index] != 0);
                ByteBlock payload;
                ok = sectionNodes[index]->getHexaText(payload, 0, MAX_PSI_LONG_SECTION_PAYLOAD_SIZE);
                if (ok) {
                    table->addSection(SectionPtr(new Section(tid, priv, tidExt, version, current, uint8_t(index), uint8_t(index), payload.data(), payload.size())));
                }
            }
            if (ok && table->isValid()) {
                return table;
            }
        }
    }

    // At this point, the table is invalid.
    elem->report().error(u"<%s>, line %d, is not a valid table", {name, elem->lineNumber()});
    return BinaryTablePtr();
}


//----------------------------------------------------------------------------
// This method decodes a <generic_descriptor>.
//----------------------------------------------------------------------------

ts::DescriptorPtr ts::XMLTables::FromGenericDescriptorXML(const xml::Element* elem)
{
    // Silently ignore invalid parameters.
    if (elem == 0) {
        return DescriptorPtr();
    }

    // Decode XML.
    DID tag = 0xFF;
    ByteBlock payload;
    const UString name(elem->name());
    const bool ok =
        name.similar(XML_GENERIC_DESCRIPTOR) &&
        elem->getIntAttribute<DID>(tag, u"tag", true, 0xFF, 0x00, 0xFF) &&
        elem->getHexaText(payload, 0, 255);

    // Build descriptor.
    if (ok) {
        return DescriptorPtr(new Descriptor(tag, payload));
    }
    else {
        elem->report().error(u"<%s>, line %d, is not a valid descriptor", {name, elem->lineNumber()});
        return DescriptorPtr();
    }
}
