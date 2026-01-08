//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2026, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCOperationPointDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"HEVC_operation_point_descriptor"
#define MY_CLASS    ts::HEVCOperationPointDescriptor
#define MY_EDID     ts::EDID::ExtensionMPEG(ts::XDID_MPEG_HEVC_OP_POINT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);

constexpr auto   MAX_PROFILE_TIER_LEVELS = 0x3F;          // 6 bits for the num_ptl
constexpr auto   MAX_OPERATION_POINTS = 0xFF;             // 8 bits for the operation_points_count
constexpr auto   MAX_ES_POINTS = 0xFF;                    // 8 bits for ES_count
constexpr auto   MAX_numEsInOp = 0x3F;                    // 6 bits for numEsInOp
constexpr size_t PROFILE_TIER_LEVEL_INFO_SIZE = 96 / 8;   // number of bytes for 96 bits


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCOperationPointDescriptor::HEVCOperationPointDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::HEVCOperationPointDescriptor::HEVCOperationPointDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCOperationPointDescriptor()
{
    deserialize(duck, desc);
}

void ts::HEVCOperationPointDescriptor::clearContent()
{
    profile_tier_level_infos.clear();
    operation_points.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCOperationPointDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(profile_tier_level_infos.size(), 6);
    for (const auto& it : profile_tier_level_infos) {
        buf.putBytes(it);
    }
    buf.putBits(operation_points.size(), 8);
    for (const auto& it : operation_points) {
        buf.putUInt8(it.target_ols);
        buf.putBits(it.ESs.size(), 8);
        for (const auto& it2 : it.ESs) {
            buf.putBits(0xFF, 1);
            buf.putBit(it2.prepend_dependencies);
            buf.putBits(it2.ES_reference, 6);
        }
        buf.putBits(0xFF, 2);
        buf.putBits(it.ESinOPs.size(), 6);
        for (const auto& it2 : it.ESinOPs) {
            buf.putBit(it2.necessary_layer_flag);
            buf.putBit(it2.output_layer_flag);
            buf.putBits(it2.ptl_ref_idx, 6);
        }
        buf.putBits(0xFF, 1);
        buf.putBit(it.avg_bit_rate.has_value());
        buf.putBit(it.max_bit_rate.has_value());
        buf.putBits(it.constant_frame_rate_info_idc, 2);
        buf.putBits(it.applicable_temporal_id, 3);
        if (it.constant_frame_rate_info_idc > 0) {
            buf.putBits(0xFF, 4);
            buf.putBits(it.frame_rate_indicator.has_value() ? it.frame_rate_indicator.value() : 0xFFFF, 12);
        }
        if (it.avg_bit_rate.has_value()) {
            buf.putBits(it.avg_bit_rate.value(), 24);
        }
        if (it.max_bit_rate.has_value()) {
            buf.putBits(it.max_bit_rate.value(), 24);
        }
    }
}


void ts::HEVCOperationPointDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    uint8_t num_ptl;
    buf.getBits(num_ptl, 6);
    for (uint8_t i = 0; i < num_ptl; i++) {
        ByteBlock newPTL = buf.getBytes(PROFILE_TIER_LEVEL_INFO_SIZE);
        profile_tier_level_infos.push_back(newPTL);
    }
    uint8_t operation_points_count = buf.getUInt8();
    for (uint8_t i = 0; i < operation_points_count; i++) {
        operation_point_type newOperationPoint;

        newOperationPoint.target_ols = buf.getUInt8();
        uint8_t ES_count = buf.getUInt8();
        for (uint8_t j = 0; j < ES_count; j++) {
            ES_type newES;
            buf.skipBits(1);
            newES.prepend_dependencies = buf.getBool();
            buf.getBits(newES.ES_reference, 6);
            newOperationPoint.ESs.push_back(newES);
        }
        buf.skipBits(2);
        uint8_t numEsInOp;
        buf.getBits(numEsInOp, 6);
        for (uint8_t k = 0; k < numEsInOp; k++) {
            ES_in_OP_type newESinOP;
            newESinOP.necessary_layer_flag = buf.getBool();
            newESinOP.output_layer_flag = buf.getBool();
            buf.getBits(newESinOP.ptl_ref_idx, 6);
            newOperationPoint.ESinOPs.push_back(newESinOP);
        }
        buf.skipBits(1);
        bool avg_bit_rate_info_flag = buf.getBool();
        bool max_bit_rate_info_flag = buf.getBool();
        buf.getBits(newOperationPoint.constant_frame_rate_info_idc, 2);
        buf.getBits(newOperationPoint.applicable_temporal_id, 3);

        if (newOperationPoint.constant_frame_rate_info_idc > 0) {
            buf.skipBits(4);
            buf.getBits(newOperationPoint.frame_rate_indicator, 12);
        }
        if (avg_bit_rate_info_flag) {
            newOperationPoint.avg_bit_rate = buf.getUInt24();
        }
        if (max_bit_rate_info_flag) {
            newOperationPoint.max_bit_rate = buf.getUInt24();
        }
        operation_points.push_back(newOperationPoint);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCOperationPointDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        buf.skipReservedBits(2);
        uint8_t num_ptl;
        buf.getBits(num_ptl, 6);
        for (uint8_t i = 0; i < num_ptl; i++) {
            disp << margin << "profile_tier_level_info[" << int(i) << "] " << UString::Dump(buf.getBytes(PROFILE_TIER_LEVEL_INFO_SIZE), UString::SINGLE_LINE) << std::endl;
        }
        uint8_t operation_points_count = buf.getUInt8();
        for (uint8_t i = 0; i < operation_points_count; i++) {
            disp << margin << "operation point[ " << int(i) << "]  target OLS: " << int(buf.getUInt8()) << std::endl;

            uint8_t ES_count = buf.getUInt8();
            for (uint8_t j = 0; j < ES_count; j++) {
                buf.skipReservedBits(1);
                disp << margin << "  ES[" << int(j) << "] prepend dependencies : " << UString::TrueFalse(buf.getBool());
                disp << ", ES reference: " << buf.getBits<uint16_t>(6) << std::endl;
            }
            buf.skipReservedBits(2);
            uint8_t numEsInOp;
            buf.getBits(numEsInOp, 6);
            for (uint8_t k = 0; k < numEsInOp; k++) {
                disp << margin << "  ESinOP[" << int(k) << "] necessary layer: " << UString::TrueFalse(buf.getBool());
                disp << ", output layer: " << UString::TrueFalse(buf.getBool());
                disp << ", PTL ref index: " << buf.getBits<uint16_t>(6) << std::endl;
            }
            buf.skipReservedBits(1);
            bool avg_bit_rate_info_flag = buf.getBool();
            bool max_bit_rate_info_flag = buf.getBool();
            uint8_t constant_frame_rate_info_idx = buf.getBits<uint8_t>(2);
            disp << margin << "  Constant Frame Rate Info: " << int(constant_frame_rate_info_idx) << ", applicable temporal id: " << buf.getBits<uint16_t>(3) << std::endl;
            bool shown = false;
            if (constant_frame_rate_info_idx > 0) {
                disp << margin << "  ";
                buf.skipReservedBits(4);
                disp << "Frame rate indicator: " << buf.getBits<uint16_t>(12);
                shown = true;
            }
            if (avg_bit_rate_info_flag) {
                if (!shown) {
                    disp << margin << "  ";
                }
                else {
                    disp << ", ";
                }
                disp << "Avg. bitrate: " << buf.getUInt24();
                shown = true;
            }
            if (max_bit_rate_info_flag) {
                if (!shown) {
                    disp << margin << "  ";
                }
                else {
                    disp << ", ";
                }
                disp << "Max. bitrate: " << buf.getUInt24();
                shown = true;
            }
            if (shown) {
                disp << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HEVCOperationPointDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : profile_tier_level_infos) {
        root->addElement(u"profile_tier_level_info")->addHexaText(it);
    }
    for (const auto& it : operation_points) {
        ts::xml::Element* op = root->addElement(u"operation_point");
        op->setIntAttribute(u"target_ols", it.target_ols);

        for (auto it2 : it.ESs) {
            ts::xml::Element* es = op->addElement(u"ES");
            es->setBoolAttribute(u"prepend_dependencies", it2.prepend_dependencies);
            es->setIntAttribute(u"ES_reference", it2.ES_reference);
        }
        for (auto it2 : it.ESinOPs) {
            ts::xml::Element* esop = op->addElement(u"ESinOP");
            esop->setBoolAttribute(u"necessary_layer", it2.necessary_layer_flag);
            esop->setBoolAttribute(u"output_layer", it2.output_layer_flag);
            esop->setIntAttribute(u"ptl_ref_idx", it2.ptl_ref_idx);
        }
        op->setIntAttribute(u"constant_frame_rate_info_idc", it.constant_frame_rate_info_idc);
        op->setIntAttribute(u"applicable_temporal_id", it.applicable_temporal_id);
        op->setOptionalIntAttribute(u"frame_rate_indicator", it.frame_rate_indicator);
        op->setOptionalIntAttribute(u"avg_bit_rate", it.avg_bit_rate);
        op->setOptionalIntAttribute(u"max_bit_rate", it.max_bit_rate);
    }
}

bool ts::HEVCOperationPointDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;

    for (auto& child : element->children(u"profile_tier_level_info", &ok, 0, MAX_PROFILE_TIER_LEVELS)) {
        ok = child.getHexaText(profile_tier_level_infos.emplace_back(), PROFILE_TIER_LEVEL_INFO_SIZE, PROFILE_TIER_LEVEL_INFO_SIZE);
    }

    for (auto& child1 : element->children(u"operation_point", &ok, 0, MAX_OPERATION_POINTS)) {
        auto& op(operation_points.emplace_back());
        ok = child1.getIntAttribute(op.target_ols, u"target_ols", true, 0, 0, 0xFF) &&
             child1.getIntAttribute(op.constant_frame_rate_info_idc, u"constant_frame_rate_info_idc", true, 0, 0, 0x03) &&
             child1.getIntAttribute(op.applicable_temporal_id, u"applicable_temporal_id", true, 0, 0, 0x07) &&
             child1.getOptionalIntAttribute(op.frame_rate_indicator, u"frame_rate_indicator", 0, 0x0FFF) &&
             child1.getOptionalIntAttribute(op.avg_bit_rate, u"avg_bit_rate", 0, 0xFFFFFF) &&
             child1.getOptionalIntAttribute(op.max_bit_rate, u"max_bit_rate", 0, 0xFFFFFF);

        for (auto& child2 : child1.children(u"ES", &ok, 0, MAX_ES_POINTS)) {
            auto& es(op.ESs.emplace_back());
            ok = child2.getBoolAttribute(es.prepend_dependencies, u"prepend_dependencies") &&
                 child2.getIntAttribute(es.ES_reference, u"ES_reference", true, 0, 0, 0x3F);
        }

        for (auto& child2 : child1.children(u"ESinOP", &ok, 0, MAX_numEsInOp)) {
            auto& es(op.ESinOPs.emplace_back());
            ok = child2.getBoolAttribute(es.necessary_layer_flag, u"necessary_layer") &&
                 child2.getBoolAttribute(es.output_layer_flag, u"output_layer") &&
                 child2.getIntAttribute(es.ptl_ref_idx, u"ptl_ref_idx", true, 0, 0, 0x3F);
        }

        if (ok && ((op.constant_frame_rate_info_idc > 0) && !op.frame_rate_indicator.has_value())) {
            child1.report().error(u"attribute frame_rate_indicator is required when constant_frame_rate_info_idc is not zero. [<%s>, line %d]", child1.name(), child1.lineNumber());
            ok = false;
        }
    }
    return ok;
}
