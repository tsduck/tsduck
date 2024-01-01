//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2024, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAuxiliaryVideoStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"auxiliary_video_stream_descriptor"
#define MY_CLASS ts::AuxiliaryVideoStreamDescriptor
#define MY_DID ts::DID_AUX_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::clear()
{
    payload_type.clear();
    payload_size.clear();
    generic_params.reset();
    depth_params.reset();
    parallax_params.reset();
    reserved_si_message.reset();
}

ts::AuxiliaryVideoStreamDescriptor::AuxiliaryVideoStreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::AuxiliaryVideoStreamDescriptor::AuxiliaryVideoStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    AuxiliaryVideoStreamDescriptor()
{
    deserialize(duck, desc);
}

void ts::AuxiliaryVideoStreamDescriptor::clearContent()
{
    aux_video_codestreamtype = 0;
    si_messages.clear();
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::generic_params_type::clear()
{
    aux_is_bottom_field.reset();
    aux_is_interlaced.reset();
    position_offset_h = 0;
    position_offset_v = 0;
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::depth_params_type::clear()
{
    nkfar = 0;
    nknear = 0;
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::parallax_params_type::clear()
{
    parallax_zero = 0;
    parallax_scale = 0;
    dref = 0;
    wref = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::iso23002_2_value_coding::serialize(PSIBuffer& buf) const
{
    for (auto i = 0; i < numFF_bytes; i++) {
        buf.putUInt8(0xFF);
    }
    buf.putUInt8(last_byte);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::generic_params_type::serialize(PSIBuffer& buf) const
{
    bool aux_is_one_field = aux_is_bottom_field.has_value();
    buf.putBit(aux_is_one_field);
    buf.putBit(aux_is_one_field ? aux_is_bottom_field.value_or(false) : aux_is_interlaced.value_or(false));
    buf.putBits(0xFF, 6);
    buf.putUInt8(position_offset_h);
    buf.putUInt8(position_offset_v);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::depth_params_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt8(nkfar);
    buf.putUInt8(nknear);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::parallax_params_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(parallax_zero);
    buf.putUInt16(parallax_scale);
    buf.putUInt16(dref);
    buf.putUInt16(wref);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::serialize(PSIBuffer& buf) const
{
    payload_type.serialize(buf);
    payload_size.serialize(buf);
    if ((payload_type.value() == 0) || (payload_type.value() == 1)) {
        if (generic_params.has_value()) {
            generic_params.value().serialize(buf);
        }
    }
    if (payload_type.value() == 0) {
        if (depth_params.has_value()) {
            depth_params.value().serialize(buf);
        }
    }
    else if (payload_type.value() == 1) {
        if (parallax_params.has_value()) {
            parallax_params.value().serialize(buf);
        }
    }
    else {
        if (reserved_si_message.has_value()) {
            buf.putBytes(reserved_si_message.value());
        }
    }
}

void ts::AuxiliaryVideoStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(aux_video_codestreamtype);
    for (const auto& si : si_messages) {
        si.serialize(buf);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::iso23002_2_value_coding::deserialize(PSIBuffer& buf)
{
    uint8_t b;
    do {
        b = buf.getUInt8();
        if (b == 0xFF) {
            numFF_bytes++;
        }
    } while (b == 0xFF && !buf.readError());
    last_byte = b;
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::generic_params_type::deserialize(PSIBuffer& buf)
{
    bool aux_is_one_field = buf.getBool();
    if (aux_is_one_field) {
        aux_is_bottom_field = buf.getBool();
    }
    else {
        aux_is_interlaced = buf.getBool();
    }
    buf.skipBits(6);
    position_offset_h = buf.getUInt8();
    position_offset_v = buf.getUInt8();
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::depth_params_type::deserialize(PSIBuffer& buf)
{
    nkfar = buf.getUInt8();
    nknear = buf.getUInt8();
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::parallax_params_type::deserialize(PSIBuffer& buf)
{
    parallax_zero = buf.getUInt16();
    parallax_scale = buf.getUInt16();
    dref = buf.getUInt16();
    wref = buf.getUInt16();
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::deserialize(PSIBuffer& buf)
{
    payload_type.deserialize(buf);
    payload_size.deserialize(buf);
    if ((payload_type.value() == 0) || (payload_type.value() == 1)) {
        generic_params_type gp(buf);
        generic_params = gp;
    }
    if (payload_type.value() == 0) {
        depth_params_type dp(buf);
        depth_params = dp;
    }
    else if (payload_type.value() == 1) {
        parallax_params_type pp(buf);
        parallax_params = pp;
    }
    else {
        reserved_si_message = buf.getBytes(payload_size.value());
    }
}

void ts::AuxiliaryVideoStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    aux_video_codestreamtype = buf.getUInt8();
    while (buf.canRead()) {
        si_message_type newSImessage(buf);
        si_messages.push_back(newSImessage);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::generic_params_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool aux_is_one_field = buf.getBool();
    bool next_bool = buf.getBool();
    buf.skipReservedBits(6);
    disp << margin << (aux_is_one_field ? "bottom field" : "interlaced") << ": " << UString::TrueFalse(next_bool) << std::endl;
    disp << margin << "Position offset h: " << int(buf.getUInt8());
    disp << ", v: " << int(buf.getUInt8()) << std::endl;
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::depth_params_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    uint8_t _nkfar = buf.getUInt8();
    uint8_t _nknear = buf.getUInt8();
    disp << margin << UString::Format(u"kfar: %.5f (numerator=%d), knear: %.5f (numberator=%d)",
        { (double(_nkfar) / 16), _nkfar, (double(_nknear) / 16), _nknear }) << std::endl;
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::parallax_params_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Parallax zero: " << buf.getUInt16();
    disp << ", scale: " << buf.getUInt16() << std::endl;
    disp << margin << "wref: " << buf.getUInt16() << "cm, dref: ";
    disp << buf.getUInt16() << "cm" << std::endl;
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    si_message_type::iso23002_2_value_coding p_type(buf);
    si_message_type::iso23002_2_value_coding p_size(buf);
    disp << margin << "SI Message, type: " << p_type.value() << ", size: " << p_size.value();
    disp << std::endl;
    if ((p_type.value() == 0) || (p_type.value() == 1)) {
        si_message_type::generic_params_type gp;
        gp.display(disp, buf, margin + u" ");
    }
    if (p_type.value() == 0) {
        si_message_type::depth_params_type dp;
        dp.display(disp, buf, margin + u" ");
    }
    else if (p_type.value() == 1) {
        si_message_type::parallax_params_type pp;
        pp.display(disp, buf, margin + u" ");
    }
    else {
        disp.displayPrivateData(u" Reserved SI message", buf, p_size.value(), margin);
    }
}

void ts::AuxiliaryVideoStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Auxiliary video code stream type: 0x%X (%<d)", { buf.getUInt8() }) << std::endl;
        while (buf.canReadBytes(2)) {
            si_message_type d;
            d.display(disp, buf, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::iso23002_2_value_coding::toXML(xml::Element* root) const
{
    ts::ByteBlock bb(numFF_bytes, 0xFF);
    bb.append(last_byte);
    root->addHexaText(bb);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::generic_params_type::toXML(xml::Element* root) const
{
    root->setOptionalBoolAttribute(u"aux_is_bottom_field", aux_is_bottom_field);
    root->setOptionalBoolAttribute(u"aux_is_interlaced", aux_is_interlaced);
    root->setIntAttribute(u"position_offset_h", position_offset_h);
    root->setIntAttribute(u"position_offset_v", position_offset_v);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::depth_params_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"kfar_numerator", nkfar);
    root->setIntAttribute(u"knear_numerator", nknear);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::parallax_params_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"parallax_zero", parallax_zero);
    root->setIntAttribute(u"parallax_scale", parallax_scale);
    root->setIntAttribute(u"dref", dref);
    root->setIntAttribute(u"wref", wref);
}

void ts::AuxiliaryVideoStreamDescriptor::si_message_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"payload_type", payload_type.value(), true);
    if ((payload_type.value() == 0) || (payload_type.value() == 1)) {
        if (generic_params.has_value()) {
            generic_params.value().toXML(root->addElement(u"generic_params"));
        }
    }
    if (payload_type.value() == 0) {
        if (depth_params.has_value()) {
            depth_params.value().toXML(root->addElement(u"depth_params"));
        }
    }
    else if (payload_type.value() == 1) {
        if (parallax_params.has_value()) {
            parallax_params.value().toXML(root->addElement(u"parallax_parameters"));
        }
    }
    else {
        if (reserved_si_message.has_value()) {
            root->addHexaTextChild(u"reserved_si_message", reserved_si_message.value());
        }
    }
}

void ts::AuxiliaryVideoStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"aux_video_codestreamtype", aux_video_codestreamtype, true);
    for (const auto& m : si_messages) {
        m.toXML(root->addElement(u"si_message"));
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AuxiliaryVideoStreamDescriptor::si_message_type::iso23002_2_value_coding::fromXML(const xml::Element* element, const UString& parent_name)
{
    ByteBlock block;
    bool ok = element->getHexaText(block, 1);
    if (!ok) {
        return ok;
    }
    if (block.size() == 1) {
        numFF_bytes = 0;
        last_byte = block[0];
    }
    else {
        if (block[block.size() - 1] == 0xFF) {
            element->report().error(u"last byte of <%s> cannot be 0xFF in <%s>, line %d", {element->name(), parent_name, element->lineNumber()});
            ok = false;
        }
        else {
            numFF_bytes = 0;
            for (size_t i = 0; i < block.size() - 1; i++) {
                if (block[i] == 0xFF) {
                    numFF_bytes++;
                }
                else {
                    element->report().error(u"leading bytes of <%s> must be 0xFF in <%s>, line %d", {element->name(), parent_name, element->lineNumber()});
                    ok = false;
                }
            }
            last_byte = block[block.size() - 1];
        }
    }
    return ok;
}

bool ts::AuxiliaryVideoStreamDescriptor::si_message_type::generic_params_type::fromXML(const xml::Element* element)
{
    xml::ElementVector gp;
    bool ok = element->getChildren(gp, u"generic_params", 1, 1) &&
              gp[0]->getIntAttribute(position_offset_h, u"position_offset_h", true) &&
              gp[0]->getIntAttribute(position_offset_v, u"position_offset_v", true);

    if (ok) {
        if (gp[0]->hasAttribute(u"aux_is_bottom_field") && gp[0]->hasAttribute(u"aux_is_interlaced")) {
            element->report().error(u"only one of <aux_is_bottom_field> and <aux_is_interlaced> must be specified  in <%s>, line %d", {element->name(), element->lineNumber()});
            ok = false;
        }
        if (!gp[0]->hasAttribute(u"aux_is_bottom_field") && !gp[0]->hasAttribute(u"aux_is_interlaced")) {
            element->report().error(u"either <aux_is_bottom_field> or <aux_is_interlaced> must be specified  in <%s>, line %d", {element->name(), element->lineNumber()});
            ok = false;
        }
    }
    if (ok) {
        ok = gp[0]->getOptionalBoolAttribute(aux_is_bottom_field, u"aux_is_bottom_field") &&
             gp[0]->getOptionalBoolAttribute(aux_is_interlaced, u"aux_is_interlaced");
    }
    return ok;
}

bool ts::AuxiliaryVideoStreamDescriptor::si_message_type::depth_params_type::fromXML(const xml::Element* element)
{
    xml::ElementVector dp;
    return element->getChildren(dp, u"depth_params", 1, 1) &&
           dp[0]->getIntAttribute(nkfar, u"kfar_numerator", true) &&
           dp[0]->getIntAttribute(nknear, u"knear_numerator", true);
}

bool ts::AuxiliaryVideoStreamDescriptor::si_message_type::parallax_params_type::fromXML(const xml::Element* element)
{
    xml::ElementVector pp;
    return element->getChildren(pp, u"parallax_params", 1, 1) &&
           pp[0]->getIntAttribute(parallax_zero, u"parallax_zero", true) &&
           pp[0]->getIntAttribute(parallax_scale, u"parallax_scale", true) &&
           pp[0]->getIntAttribute(dref, u"dref", true) &&
           pp[0]->getIntAttribute(wref, u"wref", true);
}

bool ts::AuxiliaryVideoStreamDescriptor::si_message_type::fromXML(const xml::Element* element)
{
    uint32_t ptype;
    bool ok = element->getIntAttribute(ptype, u"payload_type", true);
    if (ok) {
        payload_type.set_value(ptype);
    }
    if ((payload_type.value() == 0) || (payload_type.value() == 1)) {
        generic_params_type gp;
        if (gp.fromXML(element)) {
            generic_params = gp;
        }
        else {
            ok = false;
        }
        if (element->hasChildElement(u"reserved_si_message")) {
            element->report().error(u"<reserved_si_message> is not permitted for known payload types (0, 1)  in <%s>, line %d", {element->name(), element->lineNumber()});
            ok = false;
        }
    }
    if (payload_type.value() == 0) {
        if (element->hasChildElement(u"parallax_params")) {
            element->report().error(u"<parallax_params> is not permitted for payload type==0  in <%s>, line %d", {element->name(), element->lineNumber()});
            ok = false;
        }
        depth_params_type dp;
        if (dp.fromXML(element)) {
            depth_params = dp;
        }
        else {
            ok = false;
        }
    }
    else if (payload_type.value() == 1) {
        if (element->hasChildElement(u"depth_params")) {
            element->report().error(u"<depth_params> is not permitted for known payload type==1  in <%s>, line %d", {element->name(), element->lineNumber()});
            ok = false;
        }
        parallax_params_type pp;
        if (pp.fromXML(element)) {
            parallax_params = pp;
        }
        else {
            ok = false;
        }
    }
    else {
        if (element->hasChildElement(u"generic_params") || element->hasChildElement(u"depth_params") || element->hasChildElement(u"parallax_params")) {
            element->report().error(u"generic, depth and parallax parameters are not permitted for payload type=%d  in <%s>, line %d", {payload_type.value(), element->name(), element->lineNumber()});
            ok = false;
        }
        ByteBlock bb;
        if (element->getHexaTextChild(bb, u"reserved_si_message", true, 1)) {
            reserved_si_message = bb;
            payload_size.set_value(uint32_t(bb.size()));
        }
        else {
            ok = false;
        }
    }
    return ok;
}

bool ts::AuxiliaryVideoStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    ts::xml::ElementVector si_msgs;
    bool ok = element->getIntAttribute(aux_video_codestreamtype, u"aux_video_codestreamtype", true) &&
              element->getChildren(si_msgs, u"si_message", 1);
    if (ok) {
        for (size_t i = 0; i < si_msgs.size(); i++) {
            si_message_type newSI;
            if (newSI.fromXML(si_msgs[i])) {
                si_messages.push_back(newSI);
            }
            else {
                ok = false;
            }
        }
    }
    return ok;
}
