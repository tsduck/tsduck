//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNBIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"NBIT"
#define MY_CLASS ts::NBIT
#define MY_PID ts::PID_NBIT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {ts::TID_NBIT_BODY, ts::TID_NBIT_REF}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::NBIT::NBIT(bool is_body, uint8_t vers, bool cur) :
    AbstractLongTable(TID(is_body ? TID_NBIT_BODY : TID_NBIT_REF), MY_XML_NAME, MY_STD, vers, cur),
    original_network_id(0),
    informations(this)
{
}

ts::NBIT::NBIT(const NBIT& other) :
    AbstractLongTable(other),
    original_network_id(other.original_network_id),
    informations(this, other.informations)
{
}

ts::NBIT::NBIT(DuckContext& duck, const BinaryTable& table) :
    NBIT()
{
    deserialize(duck, table);
}

ts::NBIT::Information::Information(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::NBIT::tableIdExtension() const
{
    return original_network_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::NBIT::clearContent()
{
    original_network_id = 0;
    informations.clear();
}


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::NBIT::isValidTableId(TID tid) const
{
    return tid == TID_NBIT_BODY || tid == TID_NBIT_REF;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NBIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    original_network_id = section.tableIdExtension();

    while (buf.canRead()) {
        Information& info(informations[buf.getUInt16()]);
        buf.getBits(info.information_type, 4);
        buf.getBits(info.description_body_location, 2);
        buf.skipBits(2);
        info.user_defined = buf.getUInt8();
        for (size_t count = buf.getUInt8(); !buf.error() && count > 0; count--) {
            info.key_ids.push_back(buf.getUInt16());
        }
        buf.getDescriptorListWithLength(info.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NBIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // The section payload directly starts with the list of information sets.
    for (const auto& it : informations) {
        const Information& info(it.second);

        // Binary size of this entry.
        const size_t entry_size = 5 + 2 * info.key_ids.size() + 2 + info.descs.binarySize();

        // If we are not at the beginning of the information loop, make sure that the entire
        // information set fits in the section. If it does not fit, start a new section. Huge
        // descriptions may not fit into one section, even when starting at the beginning
        // of the information loop. In that case, the information will span two sections later.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > 0) {
            // Create a new section.
            addOneSection(table, buf);
        }

        // Serialize the characteristics of the information set. Since the number of key ids is less
        // than 256 (must fit in one byte), the key id list will fit in the section. If the descriptor
        // list is too large to fit in one section, the key_id list won't be repeated in the next section.
        size_t key_count = std::min<size_t>(255, info.key_ids.size());

        for (size_t start_index = 0; ; ) {
            buf.putUInt16(it.first); // information_id
            buf.putBits(info.information_type, 4);
            buf.putBits(info.description_body_location, 2);
            buf.putBits(0xFF, 2);
            buf.putUInt8(info.user_defined);
            buf.putUInt8(uint8_t(key_count));

            // Insert the key id list.
            for (size_t i = 0; i < key_count; ++i) {
                buf.putUInt16(info.key_ids[i]);
            }

            // Don't repeat the key id list if we overflow the descriptor list in another section.
            key_count = 0;

            // Insert descriptors (all or some).
            start_index = buf.putPartialDescriptorListWithLength(info.descs, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= info.descs.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this information entry.
            addOneSection(table, buf);
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a NBIT section.
//----------------------------------------------------------------------------

void ts::NBIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    while (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"- Information id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << "  Information type: " << DataName(MY_XML_NAME, u"InformationType", buf.getBits<uint8_t>(4), NamesFlags::FIRST) << std::endl;
        disp << margin << "  Description body location: " << DataName(MY_XML_NAME, u"DescriptionBodyLocation", buf.getBits<uint8_t>(2), NamesFlags::FIRST) << std::endl;
        buf.skipBits(2);
        disp << margin << UString::Format(u"  User defined: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        for (size_t count = buf.getUInt8(); buf.canReadBytes(2) && count > 0; count--) {
            disp << margin << UString::Format(u"  Key id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        disp.displayDescriptorListWithLength(section, buf, margin + u"  ");
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NBIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setBoolAttribute(u"body", isBody());

    for (const auto& it : informations) {
        xml::Element* e = root->addElement(u"information");
        e->setIntAttribute(u"information_id", it.first, true);
        e->setIntAttribute(u"information_type", it.second.information_type, true);
        e->setIntAttribute(u"description_body_location", it.second.description_body_location, true);
        if (it.second.user_defined != 0xFF) {
            e->setIntAttribute(u"user_defined", it.second.user_defined, true);
        }
        for (size_t i = 0; i < it.second.key_ids.size(); ++i) {
            e->addElement(u"key")->setIntAttribute(u"id", it.second.key_ids[i], true);
        }
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NBIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xinfo;
    bool body = true;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getBoolAttribute(body, u"body", false, true) &&
        element->getChildren(xinfo, u"information");

    if (body) {
        setBody();
    }
    else {
        setReference();
    }

    for (auto it1 = xinfo.begin(); ok && it1 != xinfo.end(); ++it1) {
        uint16_t id = 0;
        xml::ElementVector xkey;
        ok = (*it1)->getIntAttribute(id, u"information_id", true) &&
             (*it1)->getIntAttribute(informations[id].information_type, u"information_type", true, 0, 0, 15) &&
             (*it1)->getIntAttribute(informations[id].description_body_location, u"description_body_location", true, 0, 0, 3) &&
             (*it1)->getIntAttribute(informations[id].user_defined, u"user_defined", false, 0xFF) &&
             informations[id].descs.fromXML(duck, xkey, *it1, u"key");
        for (auto it2 = xkey.begin(); ok && it2 != xkey.end(); ++it2) {
            uint16_t kid = 0;
            ok = (*it2)->getIntAttribute(kid, u"id", true);
            if (ok) {
                informations[id].key_ids.push_back(kid);
            }
        }
    }
    return ok;
}
