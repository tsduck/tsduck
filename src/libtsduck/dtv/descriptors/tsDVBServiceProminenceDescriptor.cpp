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

#include "tsDVBServiceProminenceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"service_prominence_descriptor"
#define MY_CLASS ts::DVBServiceProminenceDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_SERVICE_PROMINENCE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBServiceProminenceDescriptor::SOGI_region_type::SOGI_region_type() :
    country_code(),
    primary_region_code(),
    secondary_region_code(),
    tertiary_region_code()
{
}

ts::DVBServiceProminenceDescriptor::SOGI_type::SOGI_type() :
    SOGI_flag(false),
    SOGI_priority(0),
    service_id(),
    regions()
{
}

ts::DVBServiceProminenceDescriptor::DVBServiceProminenceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    SOGI_list(),
    private_data()
{
}

void ts::DVBServiceProminenceDescriptor::clearContent()
{
    SOGI_list.clear();
    private_data.clear();
}

ts::DVBServiceProminenceDescriptor::DVBServiceProminenceDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBServiceProminenceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::DVBServiceProminenceDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::serializePayload(PSIBuffer& buf) const
{
    uint8_t SOGI_list_length = 0;
    for (auto _sogi : SOGI_list) {
        SOGI_list_length += 2;      // SOGI_flag, target_region_flag, service_flag, reserved(1), SOGI_priority(12)
        if (_sogi.service_id.set()) {
            SOGI_list_length += 2;  // service_id(16)
        }
        if (!_sogi.regions.empty()) {
            SOGI_list_length += 1;  // terget_region_loop_length(8)
        }
        for (auto _region : _sogi.regions) {
            SOGI_list_length += 1;  // reserved_future_use(5), country_code_flag, region_depth(2)
            if (_region.country_code.set()) {
                SOGI_list_length += 3;  // country_code(24)
            }
            if (_region.primary_region_code.set()) {
                SOGI_list_length += 1;  // primary_region_code(8)
            }
            if (_region.secondary_region_code.set()) {
                SOGI_list_length += 1;  // secondary_region_code(8)
            }
            if (_region.tertiary_region_code.set()) {
                SOGI_list_length += 2;  // tertiary_region_code(16)
            }
        }
    }
    buf.putUInt8(SOGI_list_length);
    for (auto _sogi : SOGI_list) {
        buf.putBit(_sogi.SOGI_flag);            // SOGI_flag
        buf.putBit(!_sogi.regions.empty());     // target_region_flag
        buf.putBit(_sogi.service_id.set());     // service_flag
        buf.putBit(1);                          // reserved_future_use
        buf.putBits(_sogi.SOGI_priority, 12);   // SOGI_priority
        if (_sogi.service_id.set()) {
            buf.putUInt16(_sogi.service_id.value());    // service_id
        }
        if (!_sogi.regions.empty()) {
            uint8_t target_region_loop_length = 0;
            for (auto _region : _sogi.regions) {
                target_region_loop_length += 1;     // reserved_future_use(5), country_code_flag, region_depth(2)
                if (_region.country_code.set()) {
                    target_region_loop_length += 3;  // country_code(24)
                }
                if (_region.primary_region_code.set()) {
                    target_region_loop_length += 1;  // primary_region_code(8)
                    if (_region.secondary_region_code.set()) {
                        target_region_loop_length += 1;  // secondary_region_code(8)
                        if (_region.tertiary_region_code.set()) {
                            target_region_loop_length += 2;  // tertiary_region_code(16)
                        }
                    }
                }
            }
            buf.putUInt8(target_region_loop_length);
            for (auto _region : _sogi.regions) {
                buf.putBits(0xFF, 5);
                buf.putBit(_region.country_code.set());
                uint8_t region_depth = _region.primary_region_code.set() + _region.secondary_region_code.set() + _region.tertiary_region_code.set();
                buf.putBits(region_depth, 2);
                if (_region.country_code.set()) {
                    buf.putLanguageCode(_region.country_code.value());
                }
                if (_region.primary_region_code.set()) {
                    buf.putUInt8(_region.primary_region_code.value());
                    if (_region.secondary_region_code.set()) {
                        buf.putUInt8(_region.secondary_region_code.value());
                        if (_region.tertiary_region_code.set()) {
                            buf.putUInt16(_region.tertiary_region_code.value());
                        }
                    }
                }
            }
        }
    }
    if (!private_data.empty()) {
        buf.putBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::deserializePayload(PSIBuffer& buf)
{
    uint8_t SOGI_list_length = buf.getUInt8();
    while (SOGI_list_length > 0) {
        SOGI_type s;
        s.SOGI_flag = buf.getBool();
        bool target_region_flag = buf.getBool();
        bool service_flag = buf.getBool();
        buf.skipBits(1);
        buf.getBits(s.SOGI_priority, 12);
        SOGI_list_length -= 2;
        if (service_flag) {
            s.service_id = buf.getUInt16();
            SOGI_list_length -= 2;
        }
        if (target_region_flag) {
            uint8_t target_region_loop_length = buf.getUInt8();
            SOGI_list_length--;
            while (target_region_loop_length > 0) {
                SOGI_region_type r;
                buf.skipBits(5);
                bool country_code_flag = buf.getBool();
                uint8_t region_depth = buf.getBits<uint8_t>(2);
                target_region_loop_length--; SOGI_list_length--;
                if (country_code_flag) {
                    r.country_code = buf.getLanguageCode();
                    target_region_loop_length -= 3; SOGI_list_length -= 3;
                }
                if (region_depth >= 1) {
                    r.primary_region_code = buf.getUInt8();
                    target_region_loop_length--; SOGI_list_length--;

                    if (region_depth >= 2) {
                        r.secondary_region_code = buf.getUInt8();
                        target_region_loop_length--; SOGI_list_length--;

                        if (region_depth == 3) {
                            r.tertiary_region_code = buf.getUInt16();
                            target_region_loop_length -= 2; SOGI_list_length -=2;
                        }
                    }
                }
                s.regions.push_back(r);
            }
        }
        SOGI_list.push_back(s);
    }
    private_data = buf.getBytes();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::SOGI_type::display(TablesDisplay& disp, const UString& margin)
{
    disp << margin << "SOGI flag: " << UString::TrueFalse(SOGI_flag) << ", priority: " << SOGI_priority;
    if (service_id.set())
        disp << ", service id: " << service_id.value();
    disp << std::endl;
    for (auto r : regions) {
        bool drawn = false;
        if (r.country_code.set()) {
            disp << margin << "Country: " << r.country_code.value();
            drawn = true;
        }
        if (r.primary_region_code.set()) {
            if (!drawn) {
                disp << margin << "P";
                drawn = true;
            }
            else {
                disp << ", p";
            }
            disp << "rimary region: " << int(r.primary_region_code.value());

            if (r.secondary_region_code.set()) {
                if (!drawn) {
                    disp << margin << "S";
                    drawn = true;
                }
                else {
                    disp << ", s";
                }
                disp << "econdary region: " << int(r.secondary_region_code.value());

                if (r.tertiary_region_code.set()) {
                    if (!drawn) {
                        disp << margin << "T";
                        drawn = true;
                    }
                    else {
                        disp << ", t";
                    }
                    disp << "ertiary region: " << int(r.tertiary_region_code.value());
                }
            }
        }
        if (drawn) {
            disp << std::endl;
        }
    }
}

void ts::DVBServiceProminenceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        uint8_t SOGI_list_length = buf.getUInt8();
        while (SOGI_list_length > 0) {
            SOGI_type s;
            s.SOGI_flag = buf.getBool();
            bool target_region_flag = buf.getBool();
            bool service_flag = buf.getBool();
            buf.skipReservedBits(1);
            buf.getBits(s.SOGI_priority, 12);
            SOGI_list_length -= 2;
            if (service_flag) {
                s.service_id = buf.getUInt16();
                SOGI_list_length -= 2;
            }
            if (target_region_flag) {
                uint8_t target_region_loop_length = buf.getUInt8();
                SOGI_list_length--;
                while (target_region_loop_length > 0) {
                    SOGI_region_type r;
                    buf.skipReservedBits(5);
                    bool country_code_flag = buf.getBool();
                    uint8_t region_depth = buf.getBits<uint8_t>(2);
                    target_region_loop_length--; SOGI_list_length--;
                    if (country_code_flag) {
                        r.country_code = buf.getLanguageCode();
                        target_region_loop_length -= 3; SOGI_list_length -= 3;
                    }
                    if (region_depth >= 1) {
                        r.primary_region_code = buf.getUInt8();
                        target_region_loop_length--; SOGI_list_length--;

                        if (region_depth >= 2) {
                            r.secondary_region_code = buf.getUInt8();
                            target_region_loop_length--; SOGI_list_length--;

                            if (region_depth == 3) {
                                r.tertiary_region_code = buf.getUInt16();
                                target_region_loop_length -= 2; SOGI_list_length -= 2;
                            }
                        }
                    }
                    s.regions.push_back(r);
                }
            }
            s.display(disp, margin);
        }
        ByteBlock private_data = buf.getBytes();
        if (!private_data.empty())
            disp << margin << "private_data: " << UString::Dump(private_data, UString::SINGLE_LINE) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto _sogi : SOGI_list) {
        ts::xml::Element* s = root->addElement(u"sogi");
        s->setBoolAttribute(u"SOGI_flag", _sogi.SOGI_flag);
        s->setIntAttribute(u"SOGI_priority", _sogi.SOGI_priority);
        s->setOptionalIntAttribute(u"service_id", _sogi.service_id);
        for (auto _region : _sogi.regions) {
            ts::xml::Element* r = s->addElement(u"target_region");
            if (_region.country_code.set())
                r->setAttribute(u"country_code", _region.country_code.value());
            if (_region.primary_region_code.set()) {
                r->setIntAttribute(u"primary_region_code", _region.primary_region_code.value());
                if (_region.secondary_region_code.set()) {
                    r->setIntAttribute(u"secondary_region_code", _region.secondary_region_code.value());
                    if (_region.tertiary_region_code.set()) {
                        r->setIntAttribute(u"tertiary_region_code", _region.tertiary_region_code.value());
                    }
                }
            }
        }
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBServiceProminenceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector sogis;
    bool ok =
        element->getChildren(sogis, u"sogi") &&
        element->getHexaTextChild(private_data, u"private_data", false);

    for (auto sogi : sogis) {
        SOGI_type s;
        xml::ElementVector regions;
        ok &= sogi->getBoolAttribute(s.SOGI_flag, u"SOGI_flag", true) &&
            sogi->getIntAttribute(s.SOGI_priority, u"SOGI_priority", true, 0, 0, 0xFFF) &&
            sogi->getOptionalIntAttribute(s.service_id, u"service_id", 0, 0xFFFF) &&
            sogi->getChildren(regions, u"target_region");

        for (auto rgn : regions) {
            SOGI_region_type r;
            ok &= rgn->getOptionalAttribute(r.country_code, u"country_code", 3, 3);
            ok &= rgn->getOptionalIntAttribute(r.primary_region_code, u"primary_region_code", 0, 0xFF);
            if (ok && r.primary_region_code.set()) {
                ok &= rgn->getOptionalIntAttribute(r.secondary_region_code, u"secondary_region_code", 0, 0xFF);
                if (ok && r.secondary_region_code.set()) {
                    ok &= rgn->getOptionalIntAttribute(r.tertiary_region_code, u"tertiary_region_code", 0, 0xFFFF);
                }
            }
            if (!(r.country_code.set() || r.primary_region_code.set())) {
                rgn->report().error(u"@country_code and/or @primary_region_code must be present in a <%s>, line %d", { rgn->name(), rgn->lineNumber() });
                ok = false;
            }
            s.regions.push_back(r);
        }
        SOGI_list.push_back(s);
    }
    return ok;
}
