//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsHEVCTileSubstreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"


#define MY_XML_NAME u"HEVC_tile_substream_descriptor"
#define MY_CLASS ts::HEVCTileSubstreamDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_HEVC_TILE_SSTRM
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCTileSubstreamDescriptor::substream_type::substream_type() :
    Flag(false),
    AdditionalSubstreamID(0)
{
}

ts::HEVCTileSubstreamDescriptor::HEVCTileSubstreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    SubstreamID(0),
    PreambleFlag(),
    PatternReference(),
    Substreams(),
    ReferenceFlag(1)
{
}

void ts::HEVCTileSubstreamDescriptor::clearContent()
{
    ReferenceFlag = 1;
    SubstreamID = 0;
    PreambleFlag.clear();
    PatternReference.clear();
    Substreams.clear();
}

ts::HEVCTileSubstreamDescriptor::HEVCTileSubstreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCTileSubstreamDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::HEVCTileSubstreamDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCTileSubstreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(ReferenceFlag, 1);
    buf.putBits(SubstreamID, 7);

    if ((PreambleFlag.set() && PatternReference.set()) || !Substreams.empty()) {
        if (ReferenceFlag == 1) {
            buf.putBits(PreambleFlag.value(), 1);
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
    ReferenceFlag = buf.getBits<uint8_t>(1);
    buf.getBits(SubstreamID, 7);
    if (buf.canReadBytes(1)) {
        if (ReferenceFlag == 1) {
            PreambleFlag = buf.getBits<uint8_t>(1);
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

void ts::HEVCTileSubstreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        bool hasReferenceAndOrSubstream = buf.canReadBytes(2);
        uint8_t _ReferenceFlag = buf.getBits<uint8_t>(1);
        if (hasReferenceAndOrSubstream) {
            disp << margin << "Reference flag: " << uint16_t(_ReferenceFlag) << ", s";
        }
        else {
            disp << margin << "S";
        }

        disp << "ubstream id : " << uint16_t(buf.getBits<uint8_t>(7));
        if (buf.canReadBytes(1)) {
            if (_ReferenceFlag == 1) {
                disp << ", preamble flag: " << buf.getBits<uint16_t>(1);
                disp << ", pattern reference: " << buf.getBits<uint16_t>(7);
            }
            disp << std::endl;
            if (_ReferenceFlag != 1) {
                UStringVector substreams;
                while (buf.canReadBytes(1)) {
                    uint8_t _Flag = buf.getBits<uint8_t>(1);
                    uint8_t _AdditionalSubstreamID = buf.getBits<uint8_t>(7);
                    substreams.push_back(UString::Format(u"%d-%d", { _Flag , _AdditionalSubstreamID }));
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

    if (PreambleFlag.set() && PatternReference.set()) {
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
    xml::ElementVector Reference, Substream;
    ReferenceFlag = 0;
    bool ok =
        element->getIntAttribute(SubstreamID, u"SubstreamID", true, 0, 0, 0x7F) &&
        element->getChildren(Reference, u"Reference", 0, 1) &&
        element->getChildren(Substream, u"Substream");

    if (ok && !Reference.empty() && !Substream.empty()) {
        element->report().error(u"cannot specify both Reference and Substream in <%s>, line %d", { element->name(), element->lineNumber() });
        ok = false;
    }
    if (ok && !Reference.empty()) {
        ReferenceFlag = 1;
        uint8_t _PreambleFlag, _PatternReference;
        ok = Reference[0]->getIntAttribute(_PreambleFlag, u"PreambleFlag", true, 0, 0, 1) &&
            Reference[0]->getIntAttribute(_PatternReference, u"PatternReference", true, 0, 0, 0x7F);
        PreambleFlag = _PreambleFlag;
        PatternReference = _PatternReference;
    }
    if (ok && !Substream.empty()) {
        ReferenceFlag = 0;
        for (size_t i=0; ok && i<Substream.size(); i++) {
            substream_type newSubStream;
            ok = Substream[i]->getIntAttribute(newSubStream.Flag, u"Flag", true, 0, 0, 1) &&
                Substream[i]->getIntAttribute(newSubStream.AdditionalSubstreamID, u"AdditionalSubstreamID", true, 0, 0, 0x7F);
            if (ok) {
                Substreams.push_back(newSubStream);
            }
        }
    }
    return ok;
}
