//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMVCOperationPointDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MVC_operation_point_descriptor"
#define MY_CLASS ts::MVCOperationPointDescriptor
#define MY_DID ts::DID_MVC_OPER_POINT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MVCOperationPointDescriptor::MVCOperationPointDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::MVCOperationPointDescriptor::MVCOperationPointDescriptor(DuckContext& duck, const Descriptor& desc) :
    MVCOperationPointDescriptor()
{
    deserialize(duck, desc);
}


void ts::MVCOperationPointDescriptor::clearContent()
{
    profile_idc = 0;
    constraint_set0 = false;
    constraint_set1 = false;
    constraint_set2 = false;
    constraint_set3 = false;
    constraint_set4 = false;
    constraint_set5 = false;
    AVC_compatible_flags = 0;
    levels.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MVCOperationPointDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(profile_idc);
    buf.putBit(constraint_set0);
    buf.putBit(constraint_set1);
    buf.putBit(constraint_set2);
    buf.putBit(constraint_set3);
    buf.putBit(constraint_set4);
    buf.putBit(constraint_set5);
    buf.putBits(AVC_compatible_flags, 2);
    buf.putUInt8(uint8_t(levels.size()));

    for (const auto& it1 : levels) {
        buf.putUInt8(it1.level_idc);
        buf.putUInt8(uint8_t(it1.operation_points.size()));
        for (const auto& it2 : it1.operation_points) {
            buf.putBits(0xFF, 5);
            buf.putBits(it2.applicable_temporal_id, 3);
            buf.putUInt8(it2.num_target_output_views);
            buf.putUInt8(uint8_t(it2.ES_references.size()));
            for (auto it3 = it2.ES_references.begin(); it3 != it2.ES_references.end(); ++it3) {
                buf.putBits(0xFF, 2);
                buf.putBits(*it3, 6);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MVCOperationPointDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_idc = buf.getUInt8();
    constraint_set0 = buf.getBool();
    constraint_set1 = buf.getBool();
    constraint_set2 = buf.getBool();
    constraint_set3 = buf.getBool();
    constraint_set4 = buf.getBool();
    constraint_set5 = buf.getBool();
    buf.getBits(AVC_compatible_flags, 2);

    for (uint8_t level_count = buf.getUInt8(); level_count > 0 && buf.canRead(); --level_count) {
        Level lev;
        lev.level_idc = buf.getUInt8();
        for (uint8_t points_count = buf.getUInt8(); points_count > 0 && buf.canRead(); --points_count) {
            Point pt;
            buf.skipBits(5);
            buf.getBits(pt.applicable_temporal_id, 3);
            pt.num_target_output_views = buf.getUInt8();
            for (uint8_t ES_count = buf.getUInt8(); ES_count > 0 && buf.canRead(); --ES_count) {
                buf.skipBits(2);
                pt.ES_references.push_back(buf.getBits<uint8_t>(6));
            }
            lev.operation_points.push_back(pt);
        }
        levels.push_back(lev);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MVCOperationPointDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Profile IDC: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"Constraint set: 0:%s", {buf.getBool()});
        disp << UString::Format(u", 1:%s", {buf.getBool()});
        disp << UString::Format(u", 2:%s", {buf.getBool()});
        disp << UString::Format(u", 3:%s", {buf.getBool()});
        disp << UString::Format(u", 4:%s", {buf.getBool()});
        disp << UString::Format(u", 5:%s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"AVC compatible flags: %d", {buf.getBits<uint8_t>(2)}) << std::endl;
        uint8_t level_count = buf.getUInt8();
        disp << margin << UString::Format(u"Level count: %d", {level_count}) << std::endl;

        while (level_count-- > 0 && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"- Level IDC: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            uint8_t points_count = buf.getUInt8();
            disp << margin << UString::Format(u"  Operation points count: %d", {points_count}) << std::endl;
            while (points_count-- > 0 && buf.canReadBytes(3)) {
                buf.skipBits(5);
                disp << margin << UString::Format(u"  - Applicable temporal id: %d", {buf.getBits<uint8_t>(3)}) << std::endl;
                disp << margin << UString::Format(u"    Num target output views: %d", {buf.getUInt8()}) << std::endl;
                uint8_t ES_count = buf.getUInt8();
                disp << margin << UString::Format(u"    ES count: %d", {ES_count}) << std::endl;
                while (ES_count-- > 0 && buf.canReadBytes(1)) {
                    buf.skipBits(2);
                    disp << margin << UString::Format(u"    ES reference: 0x%X (%<d)", {buf.getBits<uint8_t>(6)}) << std::endl;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MVCOperationPointDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setBoolAttribute(u"constraint_set0", constraint_set0);
    root->setBoolAttribute(u"constraint_set1", constraint_set1);
    root->setBoolAttribute(u"constraint_set2", constraint_set2);
    root->setBoolAttribute(u"constraint_set3", constraint_set3);
    root->setBoolAttribute(u"constraint_set4", constraint_set4);
    root->setBoolAttribute(u"constraint_set5", constraint_set5);
    root->setIntAttribute(u"AVC_compatible_flags", AVC_compatible_flags, false);

    for (const auto& it1 : levels) {
        xml::Element* e1 = root->addElement(u"level");
        e1->setIntAttribute(u"level_idc", it1.level_idc, true);
        for (const auto& it2 : it1.operation_points) {
            xml::Element* e2 = e1->addElement(u"operation_point");
            e2->setIntAttribute(u"applicable_temporal_id", it2.applicable_temporal_id, false);
            e2->setIntAttribute(u"num_target_output_views", it2.num_target_output_views, false);
            for (auto it3 = it2.ES_references.begin(); it3 != it2.ES_references.end(); ++it3) {
                e2->addElement(u"ES")->setIntAttribute(u"reference", *it3, true);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MVCOperationPointDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xlevel;
    bool ok =
        element->getIntAttribute(profile_idc, u"profile_idc", true) &&
        element->getBoolAttribute(constraint_set0, u"constraint_set0", true) &&
        element->getBoolAttribute(constraint_set1, u"constraint_set1", true) &&
        element->getBoolAttribute(constraint_set2, u"constraint_set2", true) &&
        element->getBoolAttribute(constraint_set3, u"constraint_set3", true) &&
        element->getBoolAttribute(constraint_set4, u"constraint_set4", true) &&
        element->getBoolAttribute(constraint_set5, u"constraint_set5", true) &&
        element->getIntAttribute(AVC_compatible_flags, u"AVC_compatible_flags", true, 0, 0, 3) &&
        element->getChildren(xlevel, u"level");

    for (auto it1 = xlevel.begin(); ok && it1 != xlevel.end(); ++it1) {
        Level lev;
        xml::ElementVector xpoint;
        ok = (*it1)->getIntAttribute(lev.level_idc, u"level_idc", true) &&
             (*it1)->getChildren(xpoint, u"operation_point");
        for (auto it2 = xpoint.begin(); ok && it2 != xpoint.end(); ++it2) {
            Point pt;
            xml::ElementVector xes;
            ok = (*it2)->getIntAttribute(pt.applicable_temporal_id, u"applicable_temporal_id", true, 0, 0, 7) &&
                 (*it2)->getIntAttribute(pt.num_target_output_views, u"num_target_output_views", true) &&
                 (*it2)->getChildren(xes, u"ES");
            for (auto it3 = xes.begin(); ok && it3 != xes.end(); ++it3) {
                uint8_t ref = 0;
                ok = (*it3)->getIntAttribute(ref, u"reference", true, 0, 0, 0x3F);
                pt.ES_references.push_back(ref);
            }
            lev.operation_points.push_back(pt);
        }
        levels.push_back(lev);
    }
    return ok;
}
