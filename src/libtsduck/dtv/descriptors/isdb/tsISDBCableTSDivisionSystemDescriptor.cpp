//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBCableTSDivisionSystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_cable_TS_division_system_descriptor"
#define MY_CLASS    ts::ISDBCableTSDivisionSystemDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_CABLE_TS_DIV, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBCableTSDivisionSystemDescriptor::ISDBCableTSDivisionSystemDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ISDBCableTSDivisionSystemDescriptor::ISDBCableTSDivisionSystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBCableTSDivisionSystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::ISDBCableTSDivisionSystemDescriptor::clearContent()
{
    carriers.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBCableTSDivisionSystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& car : carriers) {
        buf.putBCD(car.frequency / 100, 8);  // coded in 100 Hz units
        buf.putReserved(7);
        buf.putBit(car.future_use_data.empty());
        buf.putBits(car.frame_type, 4);
        buf.putBits(car.FEC_outer, 4);
        buf.putUInt8(car.modulation);
        buf.putBCD(car.symbol_rate / 100, 7);  // coded in 100 sym/s units
        buf.putBits(car.FEC_inner, 4);
        if (!car.future_use_data.empty()) {
            buf.putUInt8(uint8_t(car.future_use_data.size()));
            buf.putBytes(car.future_use_data);
        }
        buf.putUInt8(uint8_t(car.service_id.size()));
        for (auto id : car.service_id) {
            buf.putUInt16(id);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBCableTSDivisionSystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Carrier car;
        car.frequency = 100 * buf.getBCD<uint64_t>(8);  // coded in 100 Hz units
        buf.skipReservedBits(7);
        const bool future_use_data_flag = buf.getBool();
        buf.getBits(car.frame_type, 4);
        buf.getBits(car.FEC_outer, 4);
        car.modulation = buf.getUInt8();
        car.symbol_rate = 100 * buf.getBCD<uint64_t>(7);  // coded in 100 sym/s units.
        buf.getBits(car.FEC_inner, 4);
        if (!future_use_data_flag) {
            buf.getBytes(car.future_use_data, buf.getUInt8());
        }
        int number_of_services = buf.getUInt8();
        while (number_of_services-- > 0) {
            car.service_id.push_back(buf.getUInt16());
        }
        carriers.push_back(car);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBCableTSDivisionSystemDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    size_t count = 0;
    while (buf.canReadBytes(12)) {
        disp << margin << UString::Format(u"- Carrier #%d: Frequency: %d", count++, buf.getBCD<uint32_t>(4));
        disp << UString::Format(u".%04d MHz", buf.getBCD<uint32_t>(4)) << std::endl;
        buf.skipReservedBits(7);
        const bool future_use_data_flag = buf.getBool();
        disp << margin << "  Frame type: " << DataName(MY_XML_NAME, u"frame_type", buf.getBits<uint8_t>(4), NamesFlags::HEX_VALUE_NAME)  << std::endl;
        disp << margin << "  FEC outer: " << DataName(MY_XML_NAME, u"FEC_outer", buf.getBits<uint8_t>(4), NamesFlags::HEX_VALUE_NAME)  << std::endl;
        disp << margin << "  Modulation: " << DataName(MY_XML_NAME, u"modulation", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME)  << std::endl;
        disp << margin << UString::Format(u"  Symbol rate: %d", buf.getBCD<uint32_t>(3));
        disp << UString::Format(u".%04d Msymbol/s", buf.getBCD<uint32_t>(4)) << std::endl;
        disp << margin << UString::Format(u"  FEC inner: 0x%1X", buf.getBits<uint8_t>(4)) << std::endl;
        if (!future_use_data_flag) {
            disp.displayPrivateData(u"Future use data", buf, buf.getUInt8(), margin + u"  ");
        }
        if (buf.canRead()) {
            int number_of_services = buf.getUInt8();
            while (buf.canReadBytes(2) && number_of_services-- > 0) {
                disp << margin << UString::Format(u"  Service id: %n", buf.getUInt16()) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBCableTSDivisionSystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& car : carriers) {
        xml::Element* e = root->addElement(u"carrier");
        e->setIntAttribute(u"frequency", car.frequency);
        e->setIntAttribute(u"frame_type", car.frame_type, true);
        e->setIntAttribute(u"FEC_outer", car.FEC_outer, true);
        e->setIntAttribute(u"modulation", car.modulation, true);
        e->setIntAttribute(u"symbol_rate", car.symbol_rate);
        if (car.FEC_inner != 0x0F) {
            e->setIntAttribute(u"FEC_inner", car.FEC_inner, true);
        }
        e->addHexaTextChild(u"future_use_data", car.future_use_data, true);
        for (auto id : car.service_id) {
            e->addElement(u"service")->setIntAttribute(u"id", id, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBCableTSDivisionSystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcar;
    bool ok = element->getChildren(xcar, u"carrier");

    for (const auto& xc : xcar) {
        Carrier car;
        xml::ElementVector xserv;
        ok = xc->getIntAttribute(car.frequency, u"frequency", true) &&
             xc->getIntAttribute(car.frame_type, u"frame_type", true, 0, 0, 0x0F) &&
             xc->getIntAttribute(car.FEC_outer, u"FEC_outer", true, 0, 0, 0x0F) &&
             xc->getIntAttribute(car.modulation, u"modulation", true) &&
             xc->getIntAttribute(car.symbol_rate, u"symbol_rate", true) &&
             xc->getIntAttribute(car.FEC_inner, u"FEC_inner", false, 0x0F, 0, 0x0F) &&
             xc->getHexaTextChild(car.future_use_data, u"future_use_data") &&
             xc->getChildren(xserv, u"service") &&
             ok;
        for (const auto& xs : xserv) {
            uint16_t id = 0;
            ok = xs->getIntAttribute(id, u"id") && ok;
            car.service_id.push_back(id);
        }
        carriers.push_back(car);
    }
    return ok;
}
