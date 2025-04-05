//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBAdvancedCableDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_advanced_cable_delivery_system_descriptor"
#define MY_CLASS    ts::ISDBAdvancedCableDeliverySystemDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_ADV_WDS, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBAdvancedCableDeliverySystemDescriptor::ISDBAdvancedCableDeliverySystemDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ISDBAdvancedCableDeliverySystemDescriptor::ISDBAdvancedCableDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBAdvancedCableDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::ISDBAdvancedCableDeliverySystemDescriptor::clearContent()
{
    descriptor_tag_extension = 0;
    normal_data.plp_id = 0;
    normal_data.effective_symbol_length = 0;
    normal_data.guard_interval = 0;
    normal_data.bundled_channel = 0;
    normal_data.carriers.clear();
    other_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBAdvancedCableDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(descriptor_tag_extension);
    if (descriptor_tag_extension == 0) {
        buf.putUInt8(normal_data.plp_id);
        buf.putBits(normal_data.effective_symbol_length, 3);
        buf.putBits(normal_data.guard_interval, 3);
        buf.putBits(normal_data.bundled_channel, 8);
        buf.putReserved(2);
        for (const auto& car : normal_data.carriers) {
            buf.putUInt8(car.data_slice_id);
            buf.putBCD(car.frequency / 100, 8);  // coded in 100 Hz units
            buf.putBits(car.frame_type, 2);
            buf.putBits(car.FEC_outer, 4);
            buf.putBits(car.modulation, 8);
            buf.putBits(car.FEC_inner, 4);
            buf.putReserved(6);
        }
    }
    else {
        buf.putBytes(other_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBAdvancedCableDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    descriptor_tag_extension = buf.getUInt8();
    if (descriptor_tag_extension == 0) {
        normal_data.plp_id = buf.getUInt8();
        buf.getBits(normal_data.effective_symbol_length, 3);
        buf.getBits(normal_data.guard_interval, 3);
        buf.getBits(normal_data.bundled_channel, 8);
        buf.skipReservedBits(2);
        while (buf.canRead()) {
            Carrier car;
            car.data_slice_id = buf.getUInt8();
            car.frequency = 100 * buf.getBCD<uint64_t>(8);  // coded in 100 Hz units
            buf.getBits(car.frame_type, 2);
            buf.getBits(car.FEC_outer, 4);
            buf.getBits(car.modulation, 8);
            buf.getBits(car.FEC_inner, 4);
            buf.skipReservedBits(6);
            normal_data.carriers.push_back(car);
        }
    }
    else {
        buf.getBytes(other_data);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBAdvancedCableDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canRead()) {
        const uint8_t descriptor_tag_extension = buf.getUInt8();
        disp << margin << "Tag extension: " << DataName(MY_XML_NAME, u"tag_extension", descriptor_tag_extension, NamesFlags::HEX_VALUE_NAME) << std::endl;
        if (descriptor_tag_extension == 0) {
            disp << margin << UString::Format(u"PLP id: %n", buf.getUInt8()) << std::endl;
            disp << margin << UString::Format(u"Effective symbol length: %n", buf.getBits<uint8_t>(3)) << std::endl;
            disp << margin << UString::Format(u"Guard interval: %n", buf.getBits<uint8_t>(3)) << std::endl;
            disp << margin << UString::Format(u"Bundled channel: %n", buf.getBits<uint8_t>(8)) << std::endl;
            buf.skipReservedBits(2);
            size_t count = 0;
            while (buf.canReadBytes(8)) {
                disp << margin << UString::Format(u"- Carrier #%d: Data slice id %n", count++, buf.getUInt8()) << std::endl;
                disp << margin << UString::Format(u"  Frequency: %d", buf.getBCD<uint32_t>(4));
                disp << UString::Format(u".%04d MHz", buf.getBCD<uint32_t>(4)) << std::endl;
                disp << margin << UString::Format(u"  Frame type: %n", buf.getBits<uint8_t>(2)) << std::endl;
                disp << margin << UString::Format(u"  FEC outer: %n", buf.getBits<uint8_t>(4)) << std::endl;
                disp << margin << UString::Format(u"  Modulation: %n", buf.getBits<uint8_t>(8)) << std::endl;
                disp << margin << UString::Format(u"  FEC inner: %n", buf.getBits<uint8_t>(4)) << std::endl;
                buf.skipReservedBits(6);
            }
        }
        else {
            disp.displayPrivateData(u"Data", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBAdvancedCableDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    if (descriptor_tag_extension == 0) {
        xml::Element* e = root->addElement(u"normal_data");
        e->setIntAttribute(u"plp_id", normal_data.plp_id, true);
        e->setIntAttribute(u"effective_symbol_length", normal_data.effective_symbol_length, true);
        e->setIntAttribute(u"guard_interval", normal_data.guard_interval, true);
        e->setIntAttribute(u"bundled_channel", normal_data.bundled_channel, true);
        for (const auto& car : normal_data.carriers) {
            xml::Element* xcar = e->addElement(u"carrier");
            xcar->setIntAttribute(u"data_slice_id", car.data_slice_id, true);
            xcar->setIntAttribute(u"frequency", car.frequency);
            xcar->setIntAttribute(u"frame_type", car.frame_type, true);
            xcar->setIntAttribute(u"FEC_outer", car.FEC_outer, true);
            xcar->setIntAttribute(u"modulation", car.modulation, true);
            xcar->setIntAttribute(u"FEC_inner", car.FEC_inner, true);
        }
    }
    else {
        xml::Element* e = root->addElement(u"other_data");
        e->setIntAttribute(u"descriptor_tag_extension", descriptor_tag_extension, true);
        e->addHexaText(other_data, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBAdvancedCableDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xnormal, xother;
    bool ok = element->getChildren(xnormal, u"normal_data") && element->getChildren(xother, u"other_data");
    if (ok && xnormal.size() + xother.size() != 1) {
        element->report().error(u"exactly one of <normal_data> or <other_data> must be present in <%s>, line %d", element->name(), element->lineNumber());
        return false;
    }

    if (!xnormal.empty()) {
        descriptor_tag_extension = 0;
        xml::ElementVector xcar;
        ok = xnormal[0]->getIntAttribute(normal_data.plp_id, u"plp_id", true) &&
             xnormal[0]->getIntAttribute(normal_data.effective_symbol_length, u"effective_symbol_length", true, 0, 0, 7) &&
             xnormal[0]->getIntAttribute(normal_data.guard_interval, u"guard_interval", true, 0, 0, 7) &&
             xnormal[0]->getIntAttribute(normal_data.bundled_channel, u"bundled_channel", true) &&
             xnormal[0]->getChildren(xcar, u"carrier") &&
             ok;
        for (const auto& xc : xcar) {
            Carrier car;
            ok = xc->getIntAttribute(car.data_slice_id, u"data_slice_id", true) &&
                 xc->getIntAttribute(car.frequency, u"frequency", true) &&
                 xc->getIntAttribute(car.frame_type, u"frame_type", true, 0, 0, 3) &&
                 xc->getIntAttribute(car.FEC_outer, u"FEC_outer", true, 0, 0, 0x0F) &&
                 xc->getIntAttribute(car.modulation, u"modulation", true) &&
                 xc->getIntAttribute(car.FEC_inner, u"FEC_inner", true, 0, 0, 0x0F) &&
                 ok;
            normal_data.carriers.push_back(car);
        }
    }
    else {
        ok = xother[0]->getIntAttribute(descriptor_tag_extension, u"descriptor_tag_extension", false, 0x01);
        if (descriptor_tag_extension == 0x01) {
            // earthquake warning information transmission
            ok = xother[0]->getHexaText(other_data, 26, 88) && ok;
            if (ok) {
                // Pad with 0xFF up to 88 bytes.
                other_data.resize(88, 0xFF);
            }
        }
        else {
            ok = xother[0]->getHexaText(other_data) && ok;
        }
    }
    return ok;
}
