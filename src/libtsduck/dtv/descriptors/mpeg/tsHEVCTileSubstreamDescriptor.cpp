//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2026, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCTileSubstreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"HEVC_tile_substream_descriptor"
#define MY_CLASS    ts::HEVCTileSubstreamDescriptor
#define MY_EDID     ts::EDID::ExtensionMPEG(ts::XDID_MPEG_HEVC_TILE_SSTRM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCTileSubstreamDescriptor::HEVCTileSubstreamDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::HEVCTileSubstreamDescriptor::clearContent()
{
    SubstreamID = 0;
    PreambleFlag.reset();
    PatternReference.reset();
    Substreams.clear();
}

ts::HEVCTileSubstreamDescriptor::HEVCTileSubstreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCTileSubstreamDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCTileSubstreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool ref = PreambleFlag.has_value() && PatternReference.has_value();

    buf.putBit(ref);
    buf.putBits(SubstreamID, 7);

    if ((PreambleFlag.has_value() && PatternReference.has_value()) || !Substreams.empty()) {
        if (ref) {
            buf.putBit(PreambleFlag.value());
            buf.putBits(PatternReference.value(), 7);
        }
        else {
            for (auto it : Substreams) {
                buf.putBits(it.Flag, 1);
                buf.putBits(it.AdditionalSubstreamID, 7);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HEVCTileSubstreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    const bool ref = buf.getBool();
    buf.getBits(SubstreamID, 7);
    if (buf.canReadBytes(1)) {
        if (ref) {
            PreambleFlag = buf.getBit();
            PatternReference = buf.getBits<uint8_t>(7);
        }
        else {
            while (buf.canReadBytes(1)) {
                substream_type newSubStream;
                newSubStream.Flag = buf.getBits<uint8_t>(1);
                newSubStream.AdditionalSubstreamID = buf.getBits<uint8_t>(7);
                Substreams.push_back(newSubStream);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCTileSubstreamDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        bool hasReferenceAndOrSubstream = buf.canReadBytes(2);
        const bool ref = buf.getBool();
        if (hasReferenceAndOrSubstream) {
            disp << margin << "Reference flag: " << uint16_t(ref) << ", s";
        }
        else {
            disp << margin << "S";
        }

        disp << "ubstream id : " << uint16_t(buf.getBits<uint8_t>(7));
        if (buf.canReadBytes(1)) {
            if (ref) {
                disp << ", preamble flag: " << int(buf.getBit());
                disp << ", pattern reference: " << buf.getBits<uint16_t>(7);
            }
            disp << std::endl;
            if (ref != 1) {
                UStringVector substreams;
                while (buf.canReadBytes(1)) {
                    uint8_t _Flag = buf.getBits<uint8_t>(1);
                    uint8_t _AdditionalSubstreamID = buf.getBits<uint8_t>(7);
                    substreams.push_back(UString::Format(u"%d-%d",  _Flag , _AdditionalSubstreamID ));
                }
                disp.displayVector(u"Additional Stream IDs:", substreams, margin, true, 8);
            }
        }
        else {
            disp << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HEVCTileSubstreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"SubstreamID", SubstreamID);

    if (PreambleFlag.has_value() && PatternReference.has_value()) {
        ts::xml::Element* ref = root->addElement(u"Reference");
        ref->setIntAttribute(u"PreambleFlag", PreambleFlag.value());
        ref->setIntAttribute(u"PatternReference", PatternReference.value());
    }

    for (auto it : Substreams) {
        ts::xml::Element* ss = root->addElement(u"Substream");
        ss->setIntAttribute(u"Flag", it.Flag);
        ss->setIntAttribute(u"AdditionalSubstreamID", it.AdditionalSubstreamID);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::HEVCTileSubstreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(SubstreamID, u"SubstreamID", true, 0, 0, 0x7F) ;

    for (auto& xref : element->children(u"Reference", &ok, 0, 1)) {
        // Force a value on std::optional.
        PreambleFlag = 0;
        PatternReference = 0;
        ok = xref.getIntAttribute(PreambleFlag.value(), u"PreambleFlag", true, 0, 0, 1) &&
             xref.getIntAttribute(PatternReference.value(), u"PatternReference", true, 0, 0, 0x7F);
    }

    for (auto& xsub : element->children(u"Substream", &ok)) {
        if (PreambleFlag.has_value() || PatternReference.has_value()) {
            element->report().error(u"cannot specify both Reference and Substream in <%s>, line %d", element->name(), element->lineNumber());
            ok = false;
        }
        else {
            auto& newSubStream(Substreams.emplace_back());
            ok = xsub.getIntAttribute(newSubStream.Flag, u"Flag", true, 0, 0, 1) &&
                 xsub.getIntAttribute(newSubStream.AdditionalSubstreamID, u"AdditionalSubstreamID", true, 0, 0, 0x7F);
        }
    }
    return ok;
}
