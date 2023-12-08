//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsJ2KVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"J2K_video_descriptor"
#define MY_CLASS ts::J2KVideoDescriptor
#define MY_DID ts::DID_J2K_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::J2KVideoDescriptor::J2KVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::J2KVideoDescriptor::clearContent()
{
    profile_and_level = 0;
    horizontal_size = 0;
    vertical_size = 0;
    max_bit_rate = 0;
    max_buffer_size = 0;
    DEN_frame_rate = 0;
    NUM_frame_rate = 0;
    color_specification.reset();
    still_mode = false;
    interlaced_video = false;
    colour_primaries.reset();
    transfer_characteristics.reset();
    matrix_coefficients.reset();
    video_full_range_flag.reset();
    stripe.reset();
    block.reset();
    mdm.reset();
    private_data.clear();
}

ts::J2KVideoDescriptor::J2KVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    J2KVideoDescriptor()
{
    deserialize(duck, desc);
}

ts::J2KVideoDescriptor::JPEGXS_Stripe_type::JPEGXS_Stripe_type()
{
    clearContent();
}

void ts::J2KVideoDescriptor::JPEGXS_Stripe_type::clearContent()
{
    strp_max_idx = 0;
    strp_height = 0;
}


ts::J2KVideoDescriptor::JPEGXS_Block_type::JPEGXS_Block_type()
{
    clearContent();
}

void ts::J2KVideoDescriptor::JPEGXS_Block_type::clearContent()
{
    full_horizontal_size = 0;
    full_vertical_size = 0;
    blk_width = 0;
    blk_height = 0;
    max_blk_idx_h = 0;
    max_blk_idx_v = 0;
    blk_idx_h = 0;
    blk_idx_v = 0;
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    bool extended_capability_flag = colour_primaries.has_value() && transfer_characteristics.has_value() && matrix_coefficients.has_value() && video_full_range_flag.has_value();

    buf.putBit(extended_capability_flag);
    buf.putBits(profile_and_level, 15);
    buf.putUInt32(horizontal_size);
    buf.putUInt32(vertical_size);
    buf.putUInt32(max_bit_rate);
    buf.putUInt32(max_buffer_size);
    buf.putUInt16(DEN_frame_rate);
    buf.putUInt16(NUM_frame_rate);
    if (extended_capability_flag) {
        buf.putBit(stripe.has_value());
        buf.putBit(block.has_value());
        buf.putBit(mdm.has_value());
        buf.putBits(0, 5);
    }
    else {
        buf.putUInt8(color_specification.value_or(0));
    }
    buf.putBit(still_mode);
    buf.putBit(interlaced_video);
    buf.putBits(0xFF, 6);
    if (extended_capability_flag) {
        buf.putUInt8(colour_primaries.value_or(0));
        buf.putUInt8(transfer_characteristics.value_or(0));
        buf.putUInt8(matrix_coefficients.value_or(0));
        buf.putBit(video_full_range_flag.value_or(0));
        buf.putBits(0xFF, 7);
        if (stripe.has_value()) {
            stripe.value().serialize(buf);
        }
        if (block.has_value()) {
            block.value().serialize(buf);
        }
        if (mdm.has_value()) {
            mdm.value().serialize(buf);
        }
    }
    buf.putBytes(private_data);
}

void ts::J2KVideoDescriptor::JPEGXS_Stripe_type::serialize(PSIBuffer& buf) const {
    buf.putUInt8(strp_max_idx);
    buf.putUInt16(strp_height);
}

void ts::J2KVideoDescriptor::JPEGXS_Block_type::serialize(PSIBuffer& buf) const {
    buf.putUInt32(full_horizontal_size);
    buf.putUInt32(full_vertical_size);
    buf.putUInt16(blk_width);
    buf.putUInt16(blk_height);
    buf.putUInt8(max_blk_idx_h);
    buf.putUInt8(max_blk_idx_v);
    buf.putUInt8(blk_idx_h);
    buf.putUInt8(blk_idx_v);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    bool stripe_flag = false, block_flag = false, mdm_flag = false;
    bool extended_capability_flag = buf.getBool();
    buf.getBits(profile_and_level, 15);
    horizontal_size = buf.getUInt32();
    vertical_size = buf.getUInt32();
    max_bit_rate = buf.getUInt32();
    max_buffer_size = buf.getUInt32();
    DEN_frame_rate = buf.getUInt16();
    NUM_frame_rate = buf.getUInt16();
    if (extended_capability_flag) {
        stripe_flag = buf.getBool();
        block_flag = buf.getBool();
        mdm_flag = buf.getBool();
        buf.skipBits(5);
    }
    else {
        color_specification = buf.getUInt8();
    }
    still_mode = buf.getBool();
    interlaced_video = buf.getBool();
    buf.skipBits(6);
    if (extended_capability_flag) {
        colour_primaries = buf.getUInt8();
        transfer_characteristics = buf.getUInt8();
        matrix_coefficients = buf.getUInt8();
        video_full_range_flag = buf.getBool();
        buf.skipBits(7);
        if (stripe_flag) {
            JPEGXS_Stripe_type newStripe(buf);
            stripe = newStripe;
        }
        if (block_flag) {
            JPEGXS_Block_type newBlock(buf);
            block = newBlock;
        }
        if (mdm_flag) {
            Mastering_Display_Metadata_type m(buf);
            mdm = m;
        }
    }
    buf.getBytes(private_data);
}


void ts::J2KVideoDescriptor::JPEGXS_Stripe_type::deserialize(PSIBuffer& buf)
{
    strp_max_idx = buf.getUInt8();
    strp_height = buf.getUInt16();
}

void ts::J2KVideoDescriptor::JPEGXS_Block_type::deserialize(PSIBuffer& buf)
{
    full_horizontal_size = buf.getUInt32();
    full_vertical_size = buf.getUInt32();
    blk_width = buf.getUInt16();
    blk_height = buf.getUInt16();
    max_blk_idx_h = buf.getUInt8();
    max_blk_idx_v = buf.getUInt8();
    blk_idx_h = buf.getUInt8();
    blk_idx_v = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(24)) {
        bool stripe_flag=false, block_flag=false, mdm_flag=false;
        bool isExtendedCapability = buf.getBool();
        disp << margin << UString::Format(u"Profile and level: 0x%X (%<d)", {{buf.getBits<uint16_t>(15)}});
        if (isExtendedCapability) {
            disp << "  [extended]";
        }
        disp << std::endl;
        disp << margin << UString::Format(u"Horizontal size: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Vertical size: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Max bit rate: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Max buffer size: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        const uint16_t DEN_frame_rate = buf.getUInt16();
        disp << margin << UString::Format(u"Frame rate: %d/%d", {buf.getUInt16(), DEN_frame_rate}) << std::endl;
        if (isExtendedCapability) {
            stripe_flag = buf.getBool();
            block_flag = buf.getBool();
            mdm_flag = buf.getBool();
            disp << margin << "Stripe flag: " << UString::TrueFalse(stripe_flag);
            disp << ", block flag: " << UString::TrueFalse(block_flag);
            disp << ", MDM flag: " << UString::TrueFalse(mdm_flag) << std::endl;
            buf.skipReservedBits(5, 0);
        }
        else {
            disp << margin << UString::Format(u"Color specification: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        disp << margin << UString::Format(u"Still mode: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Interlaced video: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(6);
        if (isExtendedCapability) {
            disp << margin << "Colour primaries: " << DataName(MY_XML_NAME, u"colour_primaries", buf.getUInt8(), NamesFlags::VALUE | NamesFlags::DECIMAL);
            disp << ", transfer characteristics: " << DataName(MY_XML_NAME, u"transfer_characteristics", buf.getUInt8(), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
            disp << margin << "Matrix coefficients: " << DataName(MY_XML_NAME, u"matrix_coefficients", buf.getUInt8(), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
            disp << margin << "Video full range: " << UString::TrueFalse(buf.getBool()) << std::endl;
            buf.skipReservedBits(7);
            if (stripe_flag) {
                JPEGXS_Stripe_type stripe;
                stripe.display(disp, buf, margin);
            }
            if (block_flag) {
                JPEGXS_Block_type block;
                block.display(disp, buf, margin);
            }
            if (mdm_flag) {
                Mastering_Display_Metadata_type t;
                t.display(disp, buf, margin);
            }
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}

void ts::J2KVideoDescriptor::JPEGXS_Stripe_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Stripe max index: " << int(buf.getUInt8());
    disp << ", height: " << buf.getUInt16() << std::endl;
}

void ts::J2KVideoDescriptor::JPEGXS_Block_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Full size horizontal: " << buf.getUInt32();
    disp << ", vertical: " << buf.getUInt32() << std::endl;
    disp << margin << "Block witdh: " << buf.getUInt16();
    disp << ", height: " << buf.getUInt16() << std::endl;
    disp << margin << "Max block index horizontal: " << int(buf.getUInt8());
    disp << ", vertical: " << int(buf.getUInt8()) << std::endl;
    disp << margin << "Current block index horizontal: " << int(buf.getUInt8());
    disp << ", vertical: " << int(buf.getUInt8()) << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    bool extended_capability_flag = colour_primaries.has_value() && transfer_characteristics.has_value() && matrix_coefficients.has_value() && video_full_range_flag.has_value();

    root->setIntAttribute(u"profile_and_level", profile_and_level, true);
    root->setIntAttribute(u"horizontal_size", horizontal_size);
    root->setIntAttribute(u"vertical_size", vertical_size);
    root->setIntAttribute(u"max_bit_rate", max_bit_rate);
    root->setIntAttribute(u"max_buffer_size", max_buffer_size);
    root->setIntAttribute(u"DEN_frame_rate", DEN_frame_rate);
    root->setIntAttribute(u"NUM_frame_rate", NUM_frame_rate);
    if (!extended_capability_flag) {
        root->setIntAttribute(u"color_specification", color_specification.value_or(0), true);
    }
    root->setBoolAttribute(u"still_mode", still_mode);
    root->setBoolAttribute(u"interlaced_video", interlaced_video);
    if (extended_capability_flag) {
        root->setIntAttribute(u"colour_primaries", colour_primaries.value_or(0));
        root->setIntAttribute(u"transfer_characteristics", transfer_characteristics.value_or(0));
        root->setIntAttribute(u"matrix_coefficients", matrix_coefficients.value_or(0));
        root->setBoolAttribute(u"video_full_range_flag", video_full_range_flag.value_or(0));
        if (stripe.has_value()) {
            stripe.value().toXML(root->addElement(u"stripe"));
        }
        if (block.has_value()) {
            block.value().toXML(root->addElement(u"block"));
        }
        if (mdm.has_value()) {
            mdm.value().toXML(root->addElement(u"mdm"));
        }
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}

void ts::J2KVideoDescriptor::JPEGXS_Stripe_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"strp_max_idx", strp_max_idx);
    root->setIntAttribute(u"strp_height", strp_height);
}

void ts::J2KVideoDescriptor::JPEGXS_Block_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"full_horizontal_size", full_horizontal_size);
    root->setIntAttribute(u"full_vertical_size", full_vertical_size);
    root->setIntAttribute(u"blk_width", blk_width);
    root->setIntAttribute(u"blk_height", blk_height);
    root->setIntAttribute(u"max_blk_idx_h", max_blk_idx_h);
    root->setIntAttribute(u"max_blk_idx_v", max_blk_idx_v);
    root->setIntAttribute(u"blk_idx_h", blk_idx_h);
    root->setIntAttribute(u"blk_idx_v", blk_idx_v);
}



//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::J2KVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    if (element->hasAttribute(u"color_specification") &&
        (element->hasAttribute(u"colour_primaries") ||
         element->hasAttribute(u"transfer_characteristics") ||
         element->hasAttribute(u"matrix_coefficients") ||
         element->hasAttribute(u"video_full_range_flag"))) {

        element->report().error(u"cannot specify both legacy (color_specification) and extended (colour_primaries, transfer_characteristics, matrix_coefficients, video_full_range_flag) attributes in <%s>, line %d", {element->name(), element->lineNumber()});
        return false;
    }

    bool isExtendedCapabilities = element->hasAttribute(u"colour_primaries") || element->hasAttribute(u"transfer_characteristics") || element->hasAttribute(u"matrix_coefficients") || element->hasAttribute(u"video_full_range_flag");
    bool ok = element->getIntAttribute(profile_and_level, u"profile_and_level", true, 0, 0, 0x7FFF) &&
            element->getIntAttribute(horizontal_size, u"horizontal_size", true) &&
            element->getIntAttribute(vertical_size, u"vertical_size", true) &&
            element->getIntAttribute(max_bit_rate, u"max_bit_rate", true) &&
            element->getIntAttribute(max_buffer_size, u"max_buffer_size", true) &&
            element->getIntAttribute(DEN_frame_rate, u"DEN_frame_rate", true) &&
            element->getIntAttribute(NUM_frame_rate, u"NUM_frame_rate", true) &&
            element->getBoolAttribute(still_mode, u"still_mode", true) &&
            element->getBoolAttribute(interlaced_video, u"interlaced_video", true) &&
            element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 26);

    if (ok) {
        if (isExtendedCapabilities) {
            uint8_t cp, tc, mc;
            bool    vf;
            ok = element->getIntAttribute(cp, u"colour_primaries", true) &&
                 element->getIntAttribute(tc, u"transfer_characteristics", true) &&
                 element->getIntAttribute(mc, u"matrix_coefficients", true) &&
                 element->getBoolAttribute(vf, u"video_full_range_flag", true);
            if (ok) {
                colour_primaries = cp;
                transfer_characteristics = tc;
                matrix_coefficients = mc;
                video_full_range_flag = vf;
            }
        }
        else {
            uint8_t cs;
            ok = element->getIntAttribute(cs, u"color_specification", true);
            if (ok) {
                color_specification = cs;
            }
        }
    }

    if (ok && isExtendedCapabilities) {
        xml::ElementVector ext_data;

        ok = element->getChildren(ext_data, u"stripe", 0, 1);
        if (ok && !ext_data.empty()) {
            JPEGXS_Stripe_type newStripe;
            ok = newStripe.fromXML(ext_data[0]);
            stripe = newStripe;
        }

        ok =element->getChildren(ext_data, u"block", 0, 1) && ok;
        if (ok && !ext_data.empty()) {
            JPEGXS_Block_type newBlock;
            ok = newBlock.fromXML(ext_data[0]);
            block = newBlock;
        }

        ok = element->getChildren(ext_data, u"mdm", 0, 1) && ok;
        if (ok && !ext_data.empty()) {
            Mastering_Display_Metadata_type newMDM;
            ok = newMDM.fromXML(ext_data[0]);
            mdm = newMDM;
        }
    }

    return ok;
}

bool ts::J2KVideoDescriptor::JPEGXS_Stripe_type::fromXML(const xml::Element* element)
{

    return element->getIntAttribute(strp_max_idx, u"strp_max_idx", true) &&
           element->getIntAttribute(strp_height, u"strp_height", true);
}

bool ts::J2KVideoDescriptor::JPEGXS_Block_type::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(full_horizontal_size, u"full_horizontal_size", true) &&
           element->getIntAttribute(full_vertical_size, u"full_vertical_size", true) &&
           element->getIntAttribute(blk_width, u"blk_width", true) &&
           element->getIntAttribute(blk_height, u"blk_height", true) &&
           element->getIntAttribute(max_blk_idx_h, u"max_blk_idx_h", true) &&
           element->getIntAttribute(max_blk_idx_v, u"max_blk_idx_v", true) &&
           element->getIntAttribute(blk_idx_h, u"blk_idx_h", true) &&
           element->getIntAttribute(blk_idx_v, u"blk_idx_v", true);
}
