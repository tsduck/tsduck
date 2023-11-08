//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCSubregionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"HEVC_subregion_descriptor"
#define MY_CLASS ts::HEVCSubregionDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_HEVC_SUBREGION
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCSubregionDescriptor::HEVCSubregionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::HEVCSubregionDescriptor::clearContent()
{
    SubstreamIDsPerLine = 0;
    TotalSubstreamIDs = 0;
    LevelFullPanorama = 0;
    SubregionLayouts.clear();
}

ts::HEVCSubregionDescriptor::HEVCSubregionDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCSubregionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::HEVCSubregionDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCSubregionDescriptor::serializePayload(PSIBuffer& buf) const
{
    bool SubstreamMarkingFlag = false;
    for (auto it : SubregionLayouts) {
        if (it.PreambleSubstreamID.has_value())
            SubstreamMarkingFlag = true;
    }
    buf.putBit(SubstreamMarkingFlag);
    buf.putBits(SubstreamIDsPerLine, 7);
    buf.putUInt8(TotalSubstreamIDs);
    buf.putUInt8(LevelFullPanorama);
    for (auto it : SubregionLayouts) {
        if (SubstreamMarkingFlag) {
            buf.putBit(1);
            buf.putBits(it.PreambleSubstreamID.value(), 7);
        }
        uint8_t SubstreamCountMinus1 = it.Patterns.empty() ? 0 : uint8_t(it.Patterns[0].SubstreamOffset.size() - 1);
        buf.putUInt8(SubstreamCountMinus1);
        buf.putUInt8(it.Level);
        buf.putUInt16(it.PictureSizeHor);
        buf.putUInt16(it.PictureSizeVer);
        buf.putBit(1);
        buf.putBits(it.Patterns.size(), 7);
        for (auto pattern : it.Patterns) {
            for (auto sof : pattern.SubstreamOffset) {
                buf.putUInt8(sof);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HEVCSubregionDescriptor::deserializePayload(PSIBuffer& buf)
{
    bool SubstreamMarkingFlag = buf.getBool();
    SubstreamIDsPerLine = buf.getBits<uint8_t>(7);
    TotalSubstreamIDs = buf.getUInt8();
    LevelFullPanorama = buf.getUInt8();
    while (buf.canReadBytes(7)) {
        subregion_layout_type newSubregionLayout;
        if (SubstreamMarkingFlag) {
            buf.skipBits(1);
            newSubregionLayout.PreambleSubstreamID = buf.getBits<uint8_t>(7);
        }
        uint8_t SubstreamCountMinus1 = buf.getUInt8();
        newSubregionLayout.Level = buf.getUInt8();
        newSubregionLayout.PictureSizeHor = buf.getUInt16();
        newSubregionLayout.PictureSizeVer = buf.getUInt16();
        buf.skipBits(1);
        uint8_t PatternCount = buf.getBits<uint8_t>(7);
        for (uint8_t j = 0; j < PatternCount; j++) {
            pattern_type newPattern;
            for (uint8_t k = 0; k <= SubstreamCountMinus1; k++) {
                newPattern.SubstreamOffset.push_back(buf.getInt8());
            }
            newSubregionLayout.Patterns.push_back(newPattern);
        }
        SubregionLayouts.push_back(newSubregionLayout);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCSubregionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        bool SubstreamMarkingFlag = buf.getBool();
        disp << margin << "Substream IDs per line: " << int(buf.getBits<uint8_t>(7));
        disp << ", total substream IDs: " << int(buf.getBits<uint8_t>(8));
        disp << ", level full panorama: " << int(buf.getBits<uint8_t>(8)) << std::endl;
        uint16_t i = 0;
        while (buf.canReadBytes(5)) {
            disp << margin << "Layout [" << i++ << "]: ";
            if (SubstreamMarkingFlag) {
                buf.skipReservedBits(1);
                disp << "Preamble substream: " << int(buf.getBits<uint8_t>(7));
            }
            uint8_t SubstreamCountMinus1 = buf.getUInt8();
            disp << (SubstreamMarkingFlag ? ", l" : "L") << "evel: " << int(buf.getUInt8());
            disp << ", picture size hor=" << buf.getUInt16();
            disp << " ver=" << buf.getUInt16() << std::endl;
            buf.skipReservedBits(1);
            uint8_t PatternCount = buf.getBits<uint8_t>(7);
            for (uint8_t j = 0; j < PatternCount; j++) {
                std::vector<int8_t> PatternOffsets;
                for (uint8_t k = 0; k <= SubstreamCountMinus1; k++) {
                    PatternOffsets.push_back(buf.getInt8());
                }
                disp.displayVector(UString::Format(u" Pattern [%d]:", { j }), PatternOffsets, margin, true, 8);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HEVCSubregionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"SubstreamIDsPerLine", SubstreamIDsPerLine);
    root->setIntAttribute(u"TotalSubstreamIDs", TotalSubstreamIDs);
    root->setIntAttribute(u"LevelFullPanorama", LevelFullPanorama);
    for (auto i : SubregionLayouts) {
        xml::Element* srl = root->addElement(u"SubregionLayout");
        srl->setOptionalIntAttribute(u"PreambleSubstreamID", i.PreambleSubstreamID);
        srl->setIntAttribute(u"Level", i.Level);
        srl->setIntAttribute(u"PictureSizeHor", i.PictureSizeHor);
        srl->setIntAttribute(u"PictureSizeVer", i.PictureSizeVer);
        for (auto j : i.Patterns) {
            xml::Element* pattern = srl->addElement(u"Pattern");
            for (auto k : j.SubstreamOffset) {
                xml::Element* sso = pattern->addElement(u"Substream");
                sso->setIntAttribute(u"offset", k);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::HEVCSubregionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector subregions;
    int8_t SubstreamMarkingFlag = -1;
    bool ok =
        element->getIntAttribute(SubstreamIDsPerLine, u"SubstreamIDsPerLine", true, 0, 0, 0x7F) &&
        element->getIntAttribute(TotalSubstreamIDs, u"TotalSubstreamIDs", true) &&
        element->getIntAttribute(LevelFullPanorama, u"LevelFullPanorama", true) &&
        element->getChildren(subregions, u"SubregionLayout");

    if (ok) {
        for (size_t i = 0; ok && i < subregions.size(); i++) {
            subregion_layout_type newSubregionLayout;
            if (SubstreamMarkingFlag == -1) {
                SubstreamMarkingFlag = (subregions[i]->hasAttribute(u"PreambleSubstreamID") ? 1 : 0);
            }
            if ((SubstreamMarkingFlag == 1) && !(subregions[i]->hasAttribute(u"PreambleSubstreamID"))) {
                subregions[i]->report().error(u"all Subregions must either contain @PreambleSubstreamID or not in <%s>, line %d", { element->name(), element->lineNumber() });
                ok = false;
            }
            xml::ElementVector patterns;
            ok &= subregions[i]->getOptionalIntAttribute(newSubregionLayout.PreambleSubstreamID, u"PreambleSubstreamID", 0, 0x7F) &&
                subregions[i]->getIntAttribute(newSubregionLayout.Level, u"Level") &&
                subregions[i]->getIntAttribute(newSubregionLayout.PictureSizeHor, u"PictureSizeHor") &&
                subregions[i]->getIntAttribute(newSubregionLayout.PictureSizeVer, u"PictureSizeVer") &&
                subregions[i]->getChildren(patterns, u"Pattern", 1);
            int substreamCount = -1;
            if (ok) {
                for (size_t j = 0; ok && j < patterns.size(); j++) {
                    pattern_type newPattern;
                    xml::ElementVector offsets;
                    ok = patterns[j]->getChildren(offsets, u"Substream", 1);

                    if (ok) {
                        // All patterns must have the same number of SubstreamOffset values;
                        if (substreamCount == -1) {
                            substreamCount = int(offsets.size());
                        }
                        else if (int(offsets.size()) != substreamCount) {
                            element->report().error(u"number of substream offsets '%d' must be the same as in the first pattern (%d) in <%s>, line %d", { offsets.size(), substreamCount, patterns[j]->name(), patterns[j]->lineNumber() });
                            ok = false;
                        }
                    }
                    if (ok) {
                        for (size_t k = 0; ok && k < offsets.size(); k++) {
                            int8_t offset;
                            ok = offsets[k]->getIntAttribute(offset, u"offset", true);
                            if (ok) {
                                newPattern.SubstreamOffset.push_back(offset);
                            }
                        }
                    }
                    newSubregionLayout.Patterns.push_back(newPattern);
                }
            }
            if (ok) {
                SubregionLayouts.push_back(newSubregionLayout);
            }
        }
    }
    return ok;
}
