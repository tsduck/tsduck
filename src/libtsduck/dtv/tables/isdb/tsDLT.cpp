//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDLT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DLT"
#define MY_CLASS ts::DLT
#define MY_TID ts::TID_DLT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DLT::DLT() :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD)
{
}

ts::DLT::DLT(DuckContext& duck, const BinaryTable& table) :
    DLT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Check if the sections of this table have a trailing CRC32.
//----------------------------------------------------------------------------

bool ts::DLT::useTrailingCRC32() const
{
    // A DLT is a short section with a CRC32.
    return true;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DLT::clearContent()
{
    maker_id = 0;
    model_id = 0;
    version_id = 0;
    Lsection_number = 0;
    last_Lsection_number = 0;
    model_info.clear();
    code_data.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DLT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // A DLT section is a short section with a CRC32. But it has already been checked
    // and removed from the buffer since DLT::useTrailingCRC32() returns true.

    maker_id = buf.getUInt8();
    model_id = buf.getUInt8();
    version_id = buf.getUInt8();
    Lsection_number = buf.getUInt16();
    last_Lsection_number = buf.getUInt16();
    buf.getBytes(model_info, MODEL_INFO_SIZE);
    buf.getBytes(code_data, CODE_DATA_SIZE);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DLT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    buf.putUInt8(maker_id);
    buf.putUInt8(model_id);
    buf.putUInt8(version_id);
    buf.putUInt16(Lsection_number);
    buf.putUInt16(last_Lsection_number);

    // Model info and code data must have fixed size. Pad them with 0xFF.
    if (model_info.size() > MODEL_INFO_SIZE || code_data.size() > CODE_DATA_SIZE) {
        buf.setUserError();
    }
    else {
        buf.putBytes(model_info);
        for (size_t i = model_info.size(); i < MODEL_INFO_SIZE; ++i) {
            buf.putUInt8(0xFF);
        }
        buf.putBytes(code_data);
        for (size_t i = code_data.size(); i < CODE_DATA_SIZE; ++i) {
            buf.putUInt8(0xFF);
        }
    }

    // A DLT section is a short section with a CRC32. But it will be
    // automatically added since DLT::useTrailingCRC32() returns true.
}


//----------------------------------------------------------------------------
// A static method to display a DLT section.
//----------------------------------------------------------------------------

void ts::DLT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"Maker id: %n", buf.getUInt8());
        disp << UString::Format(u", model: %n", buf.getUInt8());
        disp << UString::Format(u", version: %n", buf.getUInt8()) << std::endl;
        disp << margin << UString::Format(u"Lsection: %d", buf.getUInt16());
        disp << UString::Format(u", last: %d", buf.getUInt16()) << std::endl;
        disp.displayPrivateData(u"Model info", buf, MODEL_INFO_SIZE, margin);
        disp.displayPrivateData(u"Code data", buf, CODE_DATA_SIZE, margin);
        disp.displayCRC32(section, buf, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DLT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"maker_id", maker_id, true);
    root->setIntAttribute(u"model_id", model_id, true);
    root->setIntAttribute(u"version_id", version_id, true);
    root->setIntAttribute(u"Lsection_number", Lsection_number);
    root->setIntAttribute(u"last_Lsection_number", last_Lsection_number);
    root->addHexaTextChild(u"model_info", model_info, true);
    root->addHexaTextChild(u"code_data", code_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DLT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(maker_id, u"maker_id", true) &&
           element->getIntAttribute(model_id, u"model_id", true) &&
           element->getIntAttribute(version_id, u"version_id", true) &&
           element->getIntAttribute(Lsection_number, u"Lsection_number", true) &&
           element->getIntAttribute(last_Lsection_number, u"last_Lsection_number", true) &&
           element->getHexaTextChild(model_info, u"model_info", false, 0, MODEL_INFO_SIZE) &&
           element->getHexaTextChild(code_data, u"code_data", false, 0, CODE_DATA_SIZE);
}
