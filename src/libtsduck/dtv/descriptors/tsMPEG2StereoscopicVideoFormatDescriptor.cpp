//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsMPEG2StereoscopicVideoFormatDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEG2_stereoscopic_video_format_descriptor"
#define MY_CLASS ts::MPEG2StereoscopicVideoFormatDescriptor
#define MY_DID ts::DID_STEREO_VIDEO_FORMAT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEG2StereoscopicVideoFormatDescriptor::MPEG2StereoscopicVideoFormatDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    arrangement_type()
{
}

void ts::MPEG2StereoscopicVideoFormatDescriptor::clearContent()
{
    arrangement_type.clear();
}

ts::MPEG2StereoscopicVideoFormatDescriptor::MPEG2StereoscopicVideoFormatDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEG2StereoscopicVideoFormatDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG2StereoscopicVideoFormatDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(arrangement_type.set());
    buf.putBits(arrangement_type.set() ? arrangement_type.value() : 0xFF, 7);
}

void ts::MPEG2StereoscopicVideoFormatDescriptor::deserializePayload(PSIBuffer& buf)
{
    if (buf.getBool()) {
        buf.getBits(arrangement_type, 7);
    }
    else {
        buf.skipBits(7);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEG2StereoscopicVideoFormatDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        if (buf.getBool()) {
            disp << margin << UString::Format(u"Arrangement type: 0x%X (%<d)", {buf.getBits<uint8_t>(7)}) << std::endl;
        }
        else {
            buf.skipBits(7);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG2StereoscopicVideoFormatDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setOptionalIntAttribute(u"arrangement_type", arrangement_type, true);
}

bool ts::MPEG2StereoscopicVideoFormatDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getOptionalIntAttribute(arrangement_type, u"arrangement_type", 0x00, 0x7F);
}
