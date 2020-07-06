//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsDVBAC4Descriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DVB_AC4_descriptor"
#define MY_XML_NAME_LEGACY u"AC4_descriptor"
#define MY_CLASS ts::DVBAC4Descriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_AC4
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBAC4Descriptor::DVBAC4Descriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY),
    ac4_dialog_enhancement_enabled(),
    ac4_channel_mode(),
    ac4_dsi_toc(),
    additional_info()
{
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::clearContent()
{
    ac4_dialog_enhancement_enabled.clear();
    ac4_channel_mode.clear();
    ac4_dsi_toc.clear();
    additional_info.clear();
}

ts::DVBAC4Descriptor::DVBAC4Descriptor(DuckContext& duck, const Descriptor& desc) :
    DVBAC4Descriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8((ac4_dialog_enhancement_enabled.set() && ac4_channel_mode.set() ? 0x80 : 0x00) | (!ac4_dsi_toc.empty() ? 0x40 : 0x00));
    if (ac4_dialog_enhancement_enabled.set() && ac4_channel_mode.set()) {
        bbp->appendUInt8((ac4_dialog_enhancement_enabled.value() ? 0x80 : 0x00) | uint8_t((ac4_channel_mode.value() & 0x03) << 5));
    }
    if (!ac4_dsi_toc.empty()) {
        bbp->appendUInt8(uint8_t(ac4_dsi_toc.size()));
        bbp->append(ac4_dsi_toc);
    }
    bbp->append(additional_info);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 2 && data[0] == MY_EDID;

    ac4_dialog_enhancement_enabled.clear();
    ac4_channel_mode.clear();
    ac4_dsi_toc.clear();
    additional_info.clear();

    uint8_t flags = 0;

    if (_is_valid) {
        flags = data[1];
        data += 2; size -= 2;
    }

    if (_is_valid && (flags & 0x80) != 0) {
        _is_valid = size >= 1;
        if (_is_valid) {
            ac4_dialog_enhancement_enabled = (data[0] & 0x80) != 0;
            ac4_channel_mode = (data[0] >> 5) & 0x03;
            data++; size--;
        }
    }

    if (_is_valid && (flags & 0x40) != 0) {
        _is_valid = size >= 1;
        if (_is_valid) {
            const size_t toc_size = data[0];
            _is_valid = size >= 1 + toc_size;
            if (_is_valid) {
                ac4_dsi_toc.copy(data + 1, data[0]);
                data += 1 + toc_size; size -= 1 + toc_size;
            }
        }
    }

    if (_is_valid) {
        additional_info.copy(data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t flags = data[0];
        data++; size--;
        if ((flags & 0x80) != 0 && size >= 1) {
            uint8_t type = data[0];
            data++; size--;
            strm << margin
                 << UString::Format(u"Dialog enhancement enabled: %d, channel mode: %s",
                                    {(type >> 7) & 0x01, NameFromSection(u"AC4ChannelMode", (type >> 5) & 0x03, names::FIRST)})
                 << std::endl;
        }
        if ((flags & 0x40) != 0 && size >= 1) {
            const size_t toc_size = std::min<size_t>(data[0], size - 1);
            display.displayPrivateData(u"AC-4 TOC (in DSI)", data + 1, toc_size, indent);
            data += 1 + toc_size; size -= 1 + toc_size;
        }
        display.displayPrivateData(u"Additional information", data, size, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setOptionalBoolAttribute(u"ac4_dialog_enhancement_enabled", ac4_dialog_enhancement_enabled);
    root->setOptionalIntAttribute(u"ac4_channel_mode", ac4_channel_mode);
    if (!ac4_dsi_toc.empty()) {
        root->addHexaTextChild(u"ac4_dsi_toc", ac4_dsi_toc);
    }
    if (!additional_info.empty()) {
        root->addHexaTextChild(u"additional_info", additional_info);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBAC4Descriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getOptionalBoolAttribute(ac4_dialog_enhancement_enabled, u"ac4_dialog_enhancement_enabled") &&
           element->getOptionalIntAttribute<uint8_t>(ac4_channel_mode, u"ac4_channel_mode", 0, 3) &&
           element->getHexaTextChild(ac4_dsi_toc, u"ac4_dsi_toc", false, 0, MAX_DESCRIPTOR_SIZE - 6) &&
           element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 6 - ac4_dsi_toc.size());
}
