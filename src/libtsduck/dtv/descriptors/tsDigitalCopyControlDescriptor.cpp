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

#include "tsDigitalCopyControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"digital_copy_control_descriptor"
#define MY_CLASS ts::DigitalCopyControlDescriptor
#define MY_DID ts::DID_ISDB_COPY_CONTROL
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DigitalCopyControlDescriptor::DigitalCopyControlDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    digital_recording_control_data(0),
    user_defined(0),
    maximum_bitrate(),
    components()
{
}

void ts::DigitalCopyControlDescriptor::clearContent()
{
    digital_recording_control_data = 0;
    user_defined = 0;
    maximum_bitrate.clear();
    components.clear();
}

ts::DigitalCopyControlDescriptor::DigitalCopyControlDescriptor(DuckContext& duck, const Descriptor& desc) :
    DigitalCopyControlDescriptor()
{
    deserialize(duck, desc);
}

ts::DigitalCopyControlDescriptor::Component::Component() :
    component_tag(0),
    digital_recording_control_data(0),
    user_defined(0),
    maximum_bitrate()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(digital_recording_control_data << 6) |
                     (maximum_bitrate.set() ? 0x20 : 0x00) |
                     (components.empty() ? 0x00 : 0x10) |
                     (user_defined & 0x0F));
    if (maximum_bitrate.set()) {
        bbp->appendUInt8(maximum_bitrate.value());
    }
    if (!components.empty()) {
        // Placeholder for component_control_length.
        const size_t len = bbp->size();
        bbp->appendUInt8(0);
        // Serialize components.
        for (auto it = components.begin(); it != components.end(); ++it) {
            bbp->appendUInt8(it->component_tag);
            bbp->appendUInt8(uint8_t(it->digital_recording_control_data << 6) |
                             (it->maximum_bitrate.set() ? 0x30 : 0x10) |
                             (it->user_defined & 0x0F));
            if (it->maximum_bitrate.set()) {
                bbp->appendUInt8(it->maximum_bitrate.value());
            }
        }
        // Update component_control_length.
        (*bbp)[len] = uint8_t(bbp->size() - len - 1);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 1;

    maximum_bitrate.clear();
    components.clear();

    if (_is_valid) {
        // Fixed part.
        digital_recording_control_data = (data[0] >> 6) & 0x03;
        bool bitrate_flag = (data[0] & 0x20) != 0;
        bool comp_flag = (data[0] & 0x10) != 0;
        user_defined = data[0] & 0x0F;
        data++; size--;

        if (bitrate_flag) {
            _is_valid = size > 0;
            if (_is_valid) {
                maximum_bitrate = data[0];
                data++; size--;
            }
        }

        // Component loop.
        if (_is_valid) {
            if (comp_flag) {
                _is_valid = size > 0;
                if (_is_valid) {
                    const size_t len = data[0];
                    data++; size--;
                    _is_valid = len == size;
                }
            }
            else {
                _is_valid = size == 0;
            }
        }
        while (_is_valid && size >= 2) {
            Component comp;
            comp.component_tag = data[0];
            comp.digital_recording_control_data = (data[1] >> 6) & 0x03;
            bitrate_flag = (data[1] & 0x20) != 0;
            comp.user_defined = data[1] & 0x0F;
            data += 2; size -= 2;
            if (bitrate_flag) {
                _is_valid = size > 0;
                if (_is_valid) {
                    comp.maximum_bitrate = data[0];
                    data++; size--;
                }
            }
            if (_is_valid) {
                components.push_back(comp);
            }
        }
        _is_valid = _is_valid && size == 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        uint8_t rec_control = (data[0] >> 6) & 0x03;
        bool bitrate_flag = (data[0] & 0x20) != 0;
        bool comp_flag = (data[0] & 0x10) != 0;
        uint8_t user = data[0] & 0x0F;
        data++; size--;

        strm << margin << "Recording control: " << NameFromSection(u"ISDBCopyControl", rec_control, names::DECIMAL_FIRST) << std::endl
             << margin << UString::Format(u"User-defined: 0x%1X (%d)", {user, user}) << std::endl;

        if (bitrate_flag && size > 0) {
            // Bitrate unit is 1/4 Mb/s.
            strm << margin << UString::Format(u"Maximum bitrate: %d (%'d b/s)", {data[0], BitRate(data[0]) * 250000}) << std::endl;
            data++; size--;
        }
        if (comp_flag && size > 0) {
            size_t len = std::min<size_t>(data[0], size - 1);
            data++; size--;
            while (len >= 2) {
                const uint8_t tag = data[0];
                rec_control = (data[1] >> 6) & 0x03;
                bitrate_flag = (data[1] & 0x20) != 0;
                user = data[1] & 0x0F;
                data += 2; size -= 2; len -= 2;

                strm << margin << UString::Format(u"- Component tag: 0x%X (%d)", {tag, tag}) << std::endl
                     << margin << "  Recording control: " << NameFromSection(u"ISDBCopyControl", rec_control, names::DECIMAL_FIRST) << std::endl
                     << margin << UString::Format(u"  User-defined: 0x%1X (%d)", {user, user}) << std::endl;

                if (bitrate_flag && size > 0) {
                    strm << margin << UString::Format(u"  Maximum bitrate: %d (%'d b/s)", {data[0], BitRate(data[0]) * 250000}) << std::endl;
                    data++; size--; len--;
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"digital_recording_control_data", digital_recording_control_data, false);
    root->setIntAttribute(u"user_defined", user_defined, false);
    root->setOptionalIntAttribute(u"maximum_bitrate", maximum_bitrate, false);
    for (auto it = components.begin(); it != components.end(); ++it) {
        xml::Element* e = root->addElement(u"component_control");
        e->setIntAttribute(u"component_tag", it->component_tag, false);
        e->setIntAttribute(u"digital_recording_control_data", it->digital_recording_control_data, false);
        e->setIntAttribute(u"user_defined", it->user_defined, false);
        e->setOptionalIntAttribute(u"maximum_bitrate", it->maximum_bitrate, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DigitalCopyControlDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcomp;
    bool ok =
        element->getIntAttribute<uint8_t>(digital_recording_control_data, u"digital_recording_control_data", true, 0, 0x00, 0x03) &&
        element->getIntAttribute<uint8_t>(user_defined, u"user_defined", false, 0, 0x00, 0x0F) &&
        element->getOptionalIntAttribute<uint8_t>(maximum_bitrate, u"maximum_bitrate") &&
        element->getChildren(xcomp, u"component_control");

    for (size_t i = 0; ok && i < xcomp.size(); ++i) {
        Component comp;
        ok = xcomp[i]->getIntAttribute<uint8_t>(comp.component_tag, u"component_tag", true) &&
             xcomp[i]->getIntAttribute<uint8_t>(comp.digital_recording_control_data, u"digital_recording_control_data", true, 0, 0x00, 0x03) &&
             xcomp[i]->getIntAttribute<uint8_t>(comp.user_defined, u"user_defined", false, 0, 0x00, 0x0F) &&
             xcomp[i]->getOptionalIntAttribute<uint8_t>(comp.maximum_bitrate, u"maximum_bitrate");
        components.push_back(comp);
    }
    return ok;
}
