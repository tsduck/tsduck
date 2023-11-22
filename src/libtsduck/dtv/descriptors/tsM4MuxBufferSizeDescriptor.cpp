//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsM4MuxBufferSizeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"M4MuxBufferSize_descriptor"
#define MY_CLASS    ts::M4MuxBufferSizeDescriptor
#define MY_DID      ts::DID_M4MUX_BUFFER_SIZE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::M4MuxBufferSizeDescriptor::M4MuxBufferSizeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::M4MuxBufferSizeDescriptor::clearContent()
{
    DefaultM4MuxBufferDescriptor.m4MuxChnnel = 0;
    DefaultM4MuxBufferDescriptor.FB_BufferSize = 0;
    M4MuxBufferDescriptor.clear();
}

ts::M4MuxBufferSizeDescriptor::M4MuxBufferSizeDescriptor(DuckContext& duck, const Descriptor& desc) :
    M4MuxBufferSizeDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::M4MuxBufferSizeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(DefaultM4MuxBufferDescriptor.m4MuxChnnel);
    buf.putUInt24(DefaultM4MuxBufferDescriptor.FB_BufferSize);
    for (auto it : M4MuxBufferDescriptor) {
        buf.putUInt8(it.m4MuxChnnel);
        buf.putUInt24(it.FB_BufferSize);
    }
 }


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::M4MuxBufferSizeDescriptor::deserializePayload(PSIBuffer& buf)
{
    if (buf.canReadBytes(4)) {
        DefaultM4MuxBufferDescriptor.m4MuxChnnel = buf.getUInt8();
        DefaultM4MuxBufferDescriptor.FB_BufferSize = buf.getUInt24();
    }
    while (buf.canReadBytes(4)) {
        M4MuxBufferDescriptor_type newM4MuxBufferDescriptor;
        newM4MuxBufferDescriptor.m4MuxChnnel = buf.getUInt8();
        newM4MuxBufferDescriptor.FB_BufferSize = buf.getUInt24();
        M4MuxBufferDescriptor.push_back(newM4MuxBufferDescriptor);
    }
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::M4MuxBufferSizeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << " M4MuxBuffer(default) channel: " << int(buf.getUInt8());
        disp << ", size: " << buf.getUInt24() << std::endl;
    }
    uint32_t i = 0;
    while (buf.canReadBytes(4)) {
        disp << margin << " M4MuxBuffer("<< i++ <<") channel: " << int(buf.getUInt8());
        disp << ", size: " << buf.getUInt24() << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::M4MuxBufferSizeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    ts::xml::Element* _default = root->addElement(u"DefaultM4MuxBufferDescriptor");
    _default->setIntAttribute(u"m4MuxChannel", DefaultM4MuxBufferDescriptor.m4MuxChnnel);
    _default->setIntAttribute(u"FB_BufferSize", DefaultM4MuxBufferDescriptor.FB_BufferSize);

    for (auto it : M4MuxBufferDescriptor) {
        ts::xml::Element* _buffer = root->addElement(u"M4MuxBufferDescriptor");
        _buffer->setIntAttribute(u"m4MuxChannel", it.m4MuxChnnel);
        _buffer->setIntAttribute(u"FB_BufferSize", it.FB_BufferSize);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::M4MuxBufferSizeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector DefaultM4MuxBuffer;
    bool ok = element->getChildren(DefaultM4MuxBuffer, u"DefaultM4MuxBufferDescriptor", 1, 1);
    ok &= DefaultM4MuxBuffer[0]->getIntAttribute(DefaultM4MuxBufferDescriptor.m4MuxChnnel, u"m4MuxChannel", true, 0, 0, 0xFF) &&
        DefaultM4MuxBuffer[0]->getIntAttribute(DefaultM4MuxBufferDescriptor.FB_BufferSize, u"FB_BufferSize", true, 0, 0, 0xFFFFFF);

    xml::ElementVector M4MuxBuffers;
    ok &= element->getChildren(M4MuxBuffers, u"M4MuxBufferDescriptor");
    for (size_t i = 0; ok && i < M4MuxBuffers.size(); i++) {
        M4MuxBufferDescriptor_type newMuxBuffer;
        ok &= M4MuxBuffers[i]->getIntAttribute(newMuxBuffer.m4MuxChnnel, u"m4MuxChannel", true, 0, 0, 0xFF) &&
            M4MuxBuffers[i]->getIntAttribute(newMuxBuffer.FB_BufferSize, u"FB_BufferSize", true, 0, 0, 0xFFFFFF);
        M4MuxBufferDescriptor.push_back(newMuxBuffer);
    }
    return ok;
}
