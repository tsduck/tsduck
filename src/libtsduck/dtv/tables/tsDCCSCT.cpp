//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDCCSCT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DCCSCT"
#define MY_CLASS ts::DCCSCT
#define MY_TID ts::TID_DCCSCT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);

const ts::Enumeration ts::DCCSCT::UpdateTypeNames({
    {u"new_genre_category", ts::DCCSCT::new_genre_category},
    {u"new_state",          ts::DCCSCT::new_state},
    {u"new_county",         ts::DCCSCT::new_county},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DCCSCT::DCCSCT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // DCCSCT is always "current"
    updates(this),
    descs(this)
{
}

ts::DCCSCT::DCCSCT(const DCCSCT& other) :
    AbstractLongTable(other),
    dccsct_type(other.dccsct_type),
    protocol_version(other.protocol_version),
    updates(this, other.updates),
    descs(this, other.descs)
{
}

ts::DCCSCT::DCCSCT(DuckContext& duck, const BinaryTable& table) :
    DCCSCT()
{
    deserialize(duck, table);
}

ts::DCCSCT::Update::Update(const AbstractTable* table, UpdateType type) :
    EntryWithDescriptors(table),
    update_type(type)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::DCCSCT::tableIdExtension() const
{
    return dccsct_type;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DCCSCT::clearContent()
{
    dccsct_type = 0;
    protocol_version = 0;
    descs.clear();
    updates.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DCCSCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    dccsct_type = section.tableIdExtension();
    protocol_version = buf.getUInt8();

    // Loop on all update definitions.
    uint8_t updates_defined = buf.getUInt8();
    while (!buf.error() && updates_defined-- > 0) {

        // Add a new Update at the end of the list.
        Update& upd(updates.newEntry());
        upd.update_type = UpdateType(buf.getUInt8());

        // Length of the data block (depends on the update type).
        const size_t update_data_length = buf.getUInt8();
        const size_t update_data_end = buf.currentReadByteOffset() + update_data_length;

        switch (upd.update_type) {
            case new_genre_category: {
                upd.genre_category_code = buf.getUInt8();
                buf.getMultipleString(upd.genre_category_name_text);
                break;
            }
            case new_state: {
                upd.dcc_state_location_code = buf.getUInt8();
                buf.getMultipleString(upd.dcc_state_location_code_text);
                break;
            }
            case new_county: {
                upd.state_code = buf.getUInt8();
                buf.skipBits(6);
                buf.getBits(upd.dcc_county_location_code, 10);
                buf.getMultipleString(upd.dcc_county_location_code_text);
                break;
            }
            default: {
                buf.skipBytes(update_data_length);
                break;
            }
        }

        // Make sure the update data length was correctly set, skip extra data.
        if (buf.currentReadByteOffset() > update_data_end) {
            // Corrupted data.
            buf.setUserError();
            break;
        }
        else if (buf.currentReadByteOffset() < update_data_end) {
            buf.readSeek(update_data_end);
        }

        // Deserialize descriptor list for this update (10-bit length field).
        buf.getDescriptorListWithLength(upd.descs, 10);
    }

    // Get descriptor list for the global table (10-bit length field).
    buf.getDescriptorListWithLength(descs, 10);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCCSCT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // A DCCSCT is not allowed to use more than one section, see A/65, section 6.2.
    if (updates.size() > 255) {
        buf.setUserError();
        return;
    }

    buf.putUInt8(protocol_version);
    buf.putUInt8(uint8_t(updates.size()));

    // Add description of all updates.
    for (const auto& it : updates) {
        const Update& upd(it.second);
        buf.putUInt8(upd.update_type);

        // Save position of update_data_length
        buf.pushWriteSequenceWithLeadingLength(8);

        // Insert type-dependent data.
        switch (upd.update_type) {
            case new_genre_category: {
                buf.putUInt8(upd.genre_category_code);
                buf.putMultipleString(upd.genre_category_name_text);
                break;
            }
            case new_state: {
                buf.putUInt8(upd.dcc_state_location_code);
                buf.putMultipleString(upd.dcc_state_location_code_text);
                break;
            }
            case new_county: {
                buf.putUInt8(upd.state_code);
                buf.putBits(0xFF, 6);
                buf.putBits(upd.dcc_county_location_code, 10);
                buf.putMultipleString(upd.dcc_county_location_code_text);
                break;
            }
            default: {
                break;
            }
        }

        // Update update_data_length
        buf.popState();

        // Insert descriptor list for this updates (with leading 10-bit length field)
        buf.putDescriptorListWithLength(upd.descs, 0, NPOS, 10);
    }

    // Insert common descriptor list (with leading 10-bit length field)
    buf.putDescriptorListWithLength(descs, 0, NPOS, 10);
}


//----------------------------------------------------------------------------
// A static method to display a DCCSCT section.
//----------------------------------------------------------------------------

void ts::DCCSCT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    uint16_t updates_defined = 0;

    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Protocol version: %d, DCCSCT type: 0x%X", {buf.getUInt8(), section.tableIdExtension()});
        disp << UString::Format(u", number of updates: %d", {updates_defined = buf.getUInt8()}) << std::endl;
    }

    // Loop on all updates definitions.
    while (!buf.error() && updates_defined-- > 0) {

        if (!buf.canReadBytes(2)) {
            buf.setUserError();
            break;
        }

        const uint8_t utype = buf.getUInt8();
        disp << margin << UString::Format(u"- Update type: 0x%X (%s)", {utype, UpdateTypeNames.name(utype)}) << std::endl;

        // Reduce read area to update data.
        buf.pushReadSizeFromLength(8);

        // Display variable part.
        switch (utype) {
            case new_genre_category: {
                if (buf.canReadBytes(1)) {
                    disp << margin << UString::Format(u"  Genre category code: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
                    disp.displayATSCMultipleString(buf, 0, margin + u"  ", u"Genre category name: ");
                }
                break;
            }
            case new_state: {
                if (buf.canReadBytes(1)) {
                    disp << margin << UString::Format(u"  DCC state location code: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
                    disp.displayATSCMultipleString(buf, 0, margin + u"  ", u"DCC state location: ");
                }
                break;
            }
            case new_county: {
                if (buf.canReadBytes(3)) {
                    disp << margin << UString::Format(u"  State code: 0x%X (%<d)", {buf.getUInt8()});
                    buf.skipBits(6);
                    disp << UString::Format(u", DCC county location code: 0x%03X (%<d)", {buf.getBits<uint16_t>(10)}) << std::endl;
                    disp.displayATSCMultipleString(buf, 0, margin + u"  ", u"DCC county location: ");
                }
                break;
            }
            default: {
                disp.displayPrivateData(u"Update data: ", buf, NPOS, margin + u"  ");
                break;
            }
        }

        // Terminate update data.
        disp.displayPrivateData(u"Extraneous update data", buf, NPOS, margin + u"  ");
        buf.popState();

        // Display descriptor list for this update.
        disp.displayDescriptorListWithLength(section, buf, margin + u"  ", u"Descriptors for this update:", UString(), 10);
    }

    // Display descriptor list for the global table.
    disp.displayDescriptorListWithLength(section, buf, margin, u"Additional descriptors:", UString(), 10);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DCCSCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"dccsct_type", dccsct_type, true);
    descs.toXML(duck, root);

    for (const auto& upd : updates) {
        xml::Element* e = root->addElement(u"update");
        e->setEnumAttribute(UpdateTypeNames, u"update_type", upd.second.update_type);
        upd.second.descs.toXML(duck, e);
        switch (upd.second.update_type) {
            case new_genre_category: {
                e->setIntAttribute(u"genre_category_code", upd.second.genre_category_code, true);
                upd.second.genre_category_name_text.toXML(duck, e, u"genre_category_name_text", false);
                break;
            }
            case new_state: {
                e->setIntAttribute(u"dcc_state_location_code", upd.second.dcc_state_location_code, true);
                upd.second.dcc_state_location_code_text.toXML(duck, e, u"dcc_state_location_code_text", false);
                break;
            }
            case new_county: {
                e->setIntAttribute(u"state_code", upd.second.state_code, true);
                e->setIntAttribute(u"dcc_county_location_code", upd.second.dcc_county_location_code, true);
                upd.second.dcc_county_location_code_text.toXML(duck, e, u"dcc_county_location_code_text", false);
                break;
            }
            default: {
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DCCSCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute(dccsct_type, u"dccsct_type", false, 0) &&
        descs.fromXML(duck, children, element, u"update");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new Update at the end of the list.
        Update& upd(updates.newEntry());
        xml::ElementVector unused;
        ok = children[index]->getIntEnumAttribute(upd.update_type, UpdateTypeNames, u"update_type", true) &&
            children[index]->getIntAttribute(upd.genre_category_code, u"genre_category_code", upd.update_type == new_genre_category) &&
            children[index]->getIntAttribute(upd.dcc_state_location_code, u"dcc_state_location_code", upd.update_type == new_state) &&
            children[index]->getIntAttribute(upd.state_code, u"state_code", upd.update_type == new_county) &&
            children[index]->getIntAttribute(upd.dcc_county_location_code, u"dcc_county_location_code", upd.update_type == new_county, 0, 0, 0x03FF) &&
            upd.genre_category_name_text.fromXML(duck, children[index], u"genre_category_name_text", upd.update_type == new_genre_category) &&
            upd.dcc_state_location_code_text.fromXML(duck, children[index], u"dcc_state_location_code_text", upd.update_type == new_state) &&
            upd.dcc_county_location_code_text.fromXML(duck, children[index], u"dcc_county_location_code_text", upd.update_type == new_county) &&
            upd.descs.fromXML(duck, unused, children[index], u"genre_category_name_text,dcc_state_location_code_text,dcc_county_location_code_text");
    }
    return ok;
}
