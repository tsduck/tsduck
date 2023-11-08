//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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
#define MY_CLASS ts::HEVCOperationPointDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_HEVC_OP_POINT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

constexpr auto   MAX_PROFILE_TIER_LEVELS = 0x3F;          // 6 bits for the num_ptl
constexpr auto   MAX_OPERATION_POINTS = 0xFF;             // 8 bits for the operation_points_count
constexpr auto   MAX_ES_POINTS = 0xFF;                    // 8 bits for ES_count
constexpr auto   MAX_numEsInOp = 0x3F;                    // 6 bits for numEsInOp

constexpr size_t PROFILE_TIER_LEVEL_INFO_SIZE = 96 / 8;   // number of bytes for 96 bits


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCOperationPointDescriptor::HEVCOperationPointDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::HEVCOperationPointDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------



void ts::HEVCOperationPointDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(profile_tier_level_infos.size(), 6);
    for (auto it : profile_tier_level_infos) {
        buf.putBytes(it);
    }
    buf.putBits(operation_points.size(), 8);
    for (auto it : operation_points) {
        buf.putUInt8(it.target_ols);
        buf.putBits(it.ESs.size(), 8);
        for (auto it2 : it.ESs) {
            buf.putBits(0xFF, 1);
            buf.putBit(it2.prepend_dependencies);
            buf.putBits(it2.ES_reference, 6);
        }
        buf.putBits(0xFF, 2);
        buf.putBits(it.ESinOPs.size(), 6);
        for (auto it2 : it.ESinOPs) {
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

void ts::HEVCOperationPointDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
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
                disp << "Avg. bit rate: " << buf.getUInt24();
                shown = true;
            }
            if (max_bit_rate_info_flag) {
                if (!shown) {
                    disp << margin << "  ";
                }
                else {
                    disp << ", ";
                }
                disp << "Max. bit rate: " << buf.getUInt24();
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
    for (auto it : profile_tier_level_infos) {
        root->addElement(u"profile_tier_level_info")->addHexaText(it);
    }
    for (auto it : operation_points) {
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
    xml::ElementVector _profile_tier_levels, _operation_points;

    bool ok = element->getChildren(_profile_tier_levels, u"profile_tier_level_info") &&
              element->getChildren(_operation_points, u"operation_point");

    if (ok && _profile_tier_levels.size() > MAX_PROFILE_TIER_LEVELS) {
        element->report().error(u"only %d <profile_tier_level_info> elements are permitted [<%s>, line %d]", { MAX_PROFILE_TIER_LEVELS, element->name(), element->lineNumber() });
        ok = false;
    }
    if (ok && _operation_points.size() > MAX_OPERATION_POINTS) {
        element->report().error(u"only %d <operation_point> elements are permitted [<%s>, line %d]", { MAX_OPERATION_POINTS, element->name(), element->lineNumber() });
        ok = false;
    }

    for (size_t i = 0; ok && i < _profile_tier_levels.size(); i++) {
        ByteBlock ptli;
        ok &= _profile_tier_levels[i]->getHexaText(ptli);
        if (ok && (ptli.size() != PROFILE_TIER_LEVEL_INFO_SIZE)) {
            _profile_tier_levels[i]->report().error(u"<profile_tier_level_info> must contain %d bytes [<%s>, line %d]", { PROFILE_TIER_LEVEL_INFO_SIZE, _profile_tier_levels[i]->name(), _profile_tier_levels[i]->lineNumber() });
            ok = false;
        }
        profile_tier_level_infos.push_back(ptli);
    }

    for (size_t i = 0; ok && i < _operation_points.size(); i++) {
        operation_point_type op;
        ok &= _operation_points[i]->getIntAttribute(op.target_ols, u"target_ols", true, 0, 0, 0xFF);

        xml::ElementVector _ES, _ESinOP;
        ok &= _operation_points[i]->getChildren(_ES, u"ES") &&
              _operation_points[i]->getChildren(_ESinOP, u"ESinOP");

        if (ok && (_ES.size() > MAX_ES_POINTS)) {
            _operation_points[i]->report().error(u"only %d <ES> elements are permitted [<%s>, line %d]", { MAX_ES_POINTS, _operation_points[i]->name(), _operation_points[i]->lineNumber() });
            ok = false;
        }
        for (size_t j = 0; j < _ES.size(); j++) {
            ES_type newES;
            ok &= _ES[j]->getBoolAttribute(newES.prepend_dependencies, u"prepend_dependencies") &&
                  _ES[j]->getIntAttribute(newES.ES_reference, u"ES_reference", true, 0, 0, 0x3F);
            op.ESs.push_back(newES);
        }

        if (ok && (_ESinOP.size() > MAX_numEsInOp)) {
            _operation_points[i]->report().error(u"only %d <ESinOP> elements are permitted [<%s>, line %d]", { MAX_numEsInOp, _operation_points[i]->name(), _operation_points[i]->lineNumber() });
            ok = false;
        }
        for (size_t k = 0; k < _ESinOP.size(); k++) {
            ES_in_OP_type newESinOP;
            ok &= _ESinOP[k]->getBoolAttribute(newESinOP.necessary_layer_flag, u"necessary_layer") &&
                  _ESinOP[k]->getBoolAttribute(newESinOP.output_layer_flag, u"output_layer") &&
                  _ESinOP[k]->getIntAttribute(newESinOP.ptl_ref_idx, u"ptl_ref_idx", true, 0, 0, 0x3F);
            op.ESinOPs.push_back(newESinOP);
        }

        ok &= _operation_points[i]->getIntAttribute(op.constant_frame_rate_info_idc, u"constant_frame_rate_info_idc", true, 0, 0, 0x03) &&
              _operation_points[i]->getIntAttribute(op.applicable_temporal_id, u"applicable_temporal_id", true, 0, 0, 0x07) &&
              _operation_points[i]->getOptionalIntAttribute(op.frame_rate_indicator, u"frame_rate_indicator", 0, 0x0FFF) &&
              _operation_points[i]->getOptionalIntAttribute(op.avg_bit_rate, u"avg_bit_rate", 0, 0xFFFFFF) &&
              _operation_points[i]->getOptionalIntAttribute(op.max_bit_rate, u"max_bit_rate", 0, 0xFFFFFF);

        if (ok && ((op.constant_frame_rate_info_idc > 0) && !op.frame_rate_indicator.has_value())) {
            _operation_points[i]->report().error(u"attribute frame_rate_indicator is required when constant_frame_rate_info_idc is not zero. [<%s>, line %d]", { _operation_points[i]->name(), _operation_points[i]->lineNumber() });
        }

        operation_points.push_back(op);
    }
    return ok;
}
