//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2019, Anthony Delannoy
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
//
//  Representation of a sky_service_descriptor.
//  Private descriptor, must be preceeded by the BskyB PDS.
//
//----------------------------------------------------------------------------

#include "tsSkyServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"sky_service_descriptor"
#define MY_DID ts::DID_SERVICE_SKY
#define MY_PDS ts::PDS_BSKYB
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::SkyServiceDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::SkyServiceDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_FACTORY_REGISTER(ts::SkyServiceDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));


#define E_(x,y) { std::string(y), std::string(x) }
const ts::HuffmanDecodeMap ts::SkyServiceDescriptor::it_decode_map = {
#include "sky_it.dict"
};
const ts::HuffmanDecodeMap ts::SkyServiceDescriptor::uk_decode_map = {
#include "sky_uk.dict"
};
#undef E_

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SkyServiceDescriptor::SkyServiceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    unknow1(0), flags(0), description_flags(0), description()
{
    _is_valid = true;
}

ts::SkyServiceDescriptor::SkyServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    SkyServiceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SkyServiceDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt16(unknow1);

    if (flags & SKY_SERVICE_FLAG_OPT_PRES) {
        bbp->appendUInt16(flags);
    } else {
        uint8_t req_flags = (uint8_t)(flags >> 8);
        bbp->appendUInt8(req_flags);
    }
    bbp->append(duck.toDVBWithByteLength(description));

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SkyServiceDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();

        unknow1 = GetUInt16(data);
        data += 2; size -= 2;

        flags |= ((uint16_t)GetUInt8(data) << 8);
        data += 1; size -= 1;

        if (flags & SKY_SERVICE_FLAG_OPT_PRES) {
            flags |= (uint16_t)GetUInt8(data);
            data += 1; size -= 1;
        }

        description_flags = GetUInt8(data) & 0xC0;
        description = duck.fromDVBWithByteLength(data, size);
        _is_valid = size == 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

ts::UString ts::SkyServiceDescriptor::decodeHuffmanStr(const uint8_t* str, size_t size,
                                                       ts::HuffmanDecodeMap map)
{
    std::string word, ret;
    // As said previously the first two bits seems to be flags
    uint8_t i = 2;
    size_t tmp = size;

    while (tmp) {
        uint8_t mask = (1 << (8 * sizeof(char) - i - 1));
        uint8_t byte = *(str + size - tmp);

        word += !!(byte & mask) ? '1' : '0';

        if (++i >= 8) {
            i = 0;
            tmp--;
        }

        if (map.find(word) != map.end()) {
            /* found it */
            ret += map[word];
            word = "";
        }
    }

    return ts::UString::FromUTF8(ret);
}

void ts::SkyServiceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');
    uint8_t hsize = 3, description_flags;
    uint16_t unknow1 = GetUInt16(data);
    uint16_t flags = 0;

    flags |= (uint16_t)GetUInt8(data + 2) << 8;
    if (flags & SKY_SERVICE_FLAG_OPT_PRES) {
        flags |= (uint16_t)GetUInt8(data + 3);
        hsize++;
    }
    data += hsize; size -= hsize;

    description_flags = GetUInt8(data) & 0xC0;

    strm << margin
         << UString::Format(u"Unknow1: %5d (0x%04X), Flags: 0x%04X, Descr Flags: 0x%02X",
                            {unknow1, unknow1, flags, description_flags})
         << std::endl;

    if (size > 1) {
        // By default we use UK dict
        // TODO find how to dynamically use right dict.
        ts::UString huff = decodeHuffmanStr(data, size, uk_decode_map);
        strm << margin
             << u"Description: "
             << huff
             << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SkyServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"unknow1", unknow1);
    root->setIntAttribute(u"flags", flags);
    root->setIntAttribute(u"description_flags", description_flags);
    root->addElement(u"description")->addText(description);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SkyServiceDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid = checkXMLName(element) &&
        element->getIntAttribute<uint16_t>(unknow1, u"unknow1", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(flags, u"flags", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint8_t>(description_flags, u"description_flags", true, 0, 0x00, 0xC0) &&
        element->getTextChild(description, u"description");
}
