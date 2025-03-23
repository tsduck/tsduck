//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBCAStartupDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_CA_startup_descriptor"
#define MY_CLASS    ts::ISDBCAStartupDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_CA_STARTUP, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBCAStartupDescriptor::ISDBCAStartupDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ISDBCAStartupDescriptor::ISDBCAStartupDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBCAStartupDescriptor()
{
    deserialize(duck, desc);
}

void ts::ISDBCAStartupDescriptor::clearContent()
{
    CA_system_ID = 0;
    CA_program_ID = PID_NULL;
    load_indicator = 0;
    second_CA_program_ID.reset();
    second_load_indicator.reset();
    exclusion_CA_program_ID.clear();
    load_security_info.clear();
    private_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBCAStartupDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(CA_system_ID);
    buf.putPID(CA_program_ID);
    const bool second_load_flag = second_CA_program_ID.has_value() && second_load_indicator.has_value();
    buf.putBit(second_load_flag);
    buf.putBits(load_indicator, 7);
    if (second_load_flag) {
        buf.putPID(second_CA_program_ID.value());
        buf.putReserved(1);
        buf.putBits(second_load_indicator.value(), 7);
    }
    buf.putUInt8(uint8_t(exclusion_CA_program_ID.size()));
    for (auto p : exclusion_CA_program_ID) {
        buf.putPID(p);
    }
    buf.putUInt8(uint8_t(load_security_info.size()));
    buf.putBytes(load_security_info);
    buf.putBytes(private_data);
 }


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBCAStartupDescriptor::deserializePayload(PSIBuffer& buf)
{
    CA_system_ID = buf.getUInt16();
    CA_program_ID = buf.getPID();
    const bool second_load_flag = buf.getBool();
    buf.getBits(load_indicator, 7);
    if (second_load_flag) {
        second_CA_program_ID = buf.getPID();
        buf.skipReservedBits(1);
        second_load_indicator.emplace(0);
        buf.getBits(second_load_indicator.value(), 7);
    }
    exclusion_CA_program_ID.resize(buf.getUInt8());
    for (auto& p : exclusion_CA_program_ID) {
        p = buf.getPID();
    }
    buf.getBytes(load_security_info, buf.getUInt8());
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBCAStartupDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"CA system id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"CA PID: %n", buf.getPID()) << std::endl;
        const bool second_load_flag = buf.getBool();
        disp << margin << UString::Format(u"Load indicator: 0x%X", buf.getBits<uint8_t>(7)) << std::endl;
        if (second_load_flag && buf.canReadBytes(3)) {
            disp << margin << UString::Format(u"2nd CA PID: %n", buf.getPID()) << std::endl;
            buf.skipReservedBits(1);
            disp << margin << UString::Format(u"2nd load indicator: 0x%X", buf.getBits<uint8_t>(7)) << std::endl;
        }
        if (buf.canRead()) {
            const size_t exclusion_ID_num = buf.getUInt8();
            disp << margin << "Exclusion CA PID count: " << exclusion_ID_num << std::endl;
            for (size_t i = 0; buf.canReadBytes(2) && i < exclusion_ID_num; ++i) {
                disp << margin << UString::Format(u"- Exclusion CA PID: %n", buf.getPID()) << std::endl;
            }
            if (buf.canRead()) {
                disp.displayPrivateData(u"Load security info", buf, buf.getUInt8(), margin);
            }
            disp.displayPrivateData(u"Private data", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBCAStartupDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_ID", CA_system_ID, true);
    root->setIntAttribute(u"CA_program_ID", CA_program_ID, true);
    root->setIntAttribute(u"load_indicator", load_indicator, true);
    if (second_CA_program_ID.has_value() && second_load_indicator.has_value()) {
        root->setIntAttribute(u"second_CA_program_ID", second_CA_program_ID.value(), true);
        root->setIntAttribute(u"second_load_indicator", second_load_indicator.value(), true);
    }
    for (auto p : exclusion_CA_program_ID) {
        root->addElement(u"exclusion")->setIntAttribute(u"CA_program_ID", p, true);
    }
    root->addHexaTextChild(u"load_security_info", load_security_info, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBCAStartupDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xexcl;
    bool ok = element->getIntAttribute(CA_system_ID, u"CA_system_ID", true) &&
              element->getIntAttribute(CA_program_ID, u"CA_program_ID", true, 0, 0, PID_NULL) &&
              element->getIntAttribute(load_indicator, u"load_indicator", true, 0, 0, 0x7F) &&
              element->getOptionalIntAttribute(second_CA_program_ID, u"second_CA_program_ID", 0, PID_NULL) &&
              element->getOptionalIntAttribute(second_load_indicator, u"second_load_indicator", 0, 0x7F) &&
              element->getChildren(xexcl, u"exclusion") &&
              element->getHexaTextChild(load_security_info, u"load_security_info") &&
              element->getHexaTextChild(private_data, u"private_data");

    if (second_CA_program_ID.has_value() + second_load_indicator.has_value() == 1) {
        ok = false;
        element->report().error(u"attributes 'second_CA_program_ID' and 'second_load_indicator' must be both present or absent in <%s>, line %d", element->name(), element->lineNumber());
    }

    exclusion_CA_program_ID.resize(xexcl.size());
    for (size_t i = 0; i < xexcl.size() && ok; ++i) {
        ok = xexcl[i]->getIntAttribute(exclusion_CA_program_ID[i], u"CA_program_ID", true, 0, 0, PID_NULL);
    }

    return ok;
}
