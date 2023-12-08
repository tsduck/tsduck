//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

ts::DVBServiceProminenceDescriptor::DVBServiceProminenceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
    buf.pushWriteSequenceWithLeadingLength(8);  // will write SOGI_list_length here
    for (const auto& _sogi : SOGI_list) {
        buf.putBit(_sogi.SOGI_flag);            // SOGI_flag
        buf.putBit(!_sogi.regions.empty());     // target_region_flag
        buf.putBit(_sogi.service_id.has_value());     // service_flag
        buf.putReserved(1);                     // reserved_future_use
        buf.putBits(_sogi.SOGI_priority, 12);   // SOGI_priority
        if (_sogi.service_id.has_value()) {
            buf.putUInt16(_sogi.service_id.value());    // service_id
        }
        if (!_sogi.regions.empty()) {
            buf.pushWriteSequenceWithLeadingLength(8);  // will write target_region_loop_length here
            for (const auto& _region : _sogi.regions) {
                buf.putReserved(5);
                buf.putBit(_region.country_code.has_value());
                const uint8_t region_depth =
                    _region.primary_region_code.has_value() +
                    (_region.primary_region_code.has_value() && _region.secondary_region_code.has_value()) +
                    (_region.primary_region_code.has_value() && _region.secondary_region_code.has_value() && _region.tertiary_region_code.has_value());
                buf.putBits(region_depth, 2);
                if (_region.country_code.has_value()) {
                    buf.putLanguageCode(_region.country_code.value());
                }
                if (_region.primary_region_code.has_value()) {
                    buf.putUInt8(_region.primary_region_code.value());
                    if (_region.secondary_region_code.has_value()) {
                        buf.putUInt8(_region.secondary_region_code.value());
                        if (_region.tertiary_region_code.has_value()) {
                            buf.putUInt16(_region.tertiary_region_code.value());
                        }
                    }
                }
            }
            buf.popState(); // update target_region_loop_length
        }
    }
    buf.popState(); // update SOGI_list_length
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.pushReadSizeFromLength(8); // start read sequence
    while (buf.canRead()) {
        SOGI_type s;
        s.SOGI_flag = buf.getBool();
        const bool target_region_flag = buf.getBool();
        const bool service_flag = buf.getBool();
        buf.skipReservedBits(1);
        buf.getBits(s.SOGI_priority, 12);
        if (service_flag) {
            s.service_id = buf.getUInt16();
        }
        if (target_region_flag) {
            buf.pushReadSizeFromLength(8); // start read sequence
            while (buf.canRead()) {
                SOGI_region_type r;
                buf.skipReservedBits(5);
                const bool country_code_flag = buf.getBool();
                const uint8_t region_depth = buf.getBits<uint8_t>(2);
                if (country_code_flag) {
                    r.country_code = buf.getLanguageCode();
                }
                if (region_depth >= 1) {
                    r.primary_region_code = buf.getUInt8();
                    if (region_depth >= 2) {
                        r.secondary_region_code = buf.getUInt8();
                        if (region_depth == 3) {
                            r.tertiary_region_code = buf.getUInt16();
                        }
                    }
                }
                s.regions.push_back(r);
            }
            buf.popState(); // end read sequence
        }
        SOGI_list.push_back(s);
    }
    buf.popState(); // end read sequence
    private_data = buf.getBytes();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.pushReadSizeFromLength(8); // start read sequence
        while (buf.canReadBytes(2)) {
            disp << margin << "SOGI flag: " << UString::TrueFalse(buf.getBool());
            const bool target_region_flag = buf.getBool();
            const bool service_flag = buf.getBool();
            buf.skipReservedBits(1);
            disp << ", priority: " << buf.getBits<uint16_t>(12);
            if (service_flag && buf.canReadBytes(2)) {
                disp << ", service id: " << buf.getUInt16();
            }
            disp << std::endl;
            if (target_region_flag) {
                buf.pushReadSizeFromLength(8); // start read sequence
                while (buf.canReadBytes(1)) {
                    buf.skipReservedBits(5);
                    const bool country_code_flag = buf.getBool();
                    const uint8_t region_depth = buf.getBits<uint8_t>(2);
                    bool drawn = false;
                    if (country_code_flag && buf.canReadBytes(3)) {
                        disp << margin << "Country: " << buf.getLanguageCode();
                        drawn = true;
                    }
                    if (region_depth >= 1 && buf.canReadBytes(1)) {
                        if (!drawn) {
                            disp << margin << "P";
                            drawn = true;
                        }
                        else {
                            disp << ", p";
                        }
                        disp << "rimary region: " << int(buf.getUInt8());
                        if (region_depth >= 2 && buf.canReadBytes(1)) {
                            if (!drawn) {
                                disp << margin << "S";
                                drawn = true;
                            }
                            else {
                                disp << ", s";
                            }
                            disp << "econdary region: " << int(buf.getUInt8());
                            if (region_depth >= 3 && buf.canReadBytes(2)) {
                                if (!drawn) {
                                    disp << margin << "T";
                                    drawn = true;
                                }
                                else {
                                    disp << ", t";
                                }
                                disp << "ertiary region: " << buf.getUInt16();
                            }
                        }
                    }
                    if (drawn) {
                        disp << std::endl;
                    }
                }
                buf.popState(); // end read sequence
            }
        }
        buf.popState(); // end read sequence
        disp.displayPrivateData(u"private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBServiceProminenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& _sogi : SOGI_list) {
        ts::xml::Element* s = root->addElement(u"sogi");
        s->setBoolAttribute(u"SOGI_flag", _sogi.SOGI_flag);
        s->setIntAttribute(u"SOGI_priority", _sogi.SOGI_priority);
        s->setOptionalIntAttribute(u"service_id", _sogi.service_id);
        for (const auto& _region : _sogi.regions) {
            ts::xml::Element* r = s->addElement(u"target_region");
            if (_region.country_code.has_value()) {
                r->setAttribute(u"country_code", _region.country_code.value());
            }
            r->setOptionalIntAttribute(u"primary_region_code", _region.primary_region_code);
            r->setOptionalIntAttribute(u"secondary_region_code", _region.secondary_region_code);
            r->setOptionalIntAttribute(u"tertiary_region_code", _region.tertiary_region_code);
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
        ok = ok &&
             sogi->getBoolAttribute(s.SOGI_flag, u"SOGI_flag", true) &&
             sogi->getIntAttribute(s.SOGI_priority, u"SOGI_priority", true, 0, 0, 0xFFF) &&
             sogi->getOptionalIntAttribute(s.service_id, u"service_id", 0, 0xFFFF) &&
             sogi->getChildren(regions, u"target_region");

        for (auto rgn : regions) {
            SOGI_region_type r;
            ok = ok &&
                 rgn->getOptionalAttribute(r.country_code, u"country_code", 3, 3) &&
                 rgn->getOptionalIntAttribute(r.primary_region_code, u"primary_region_code", 0, 0xFF) &&
                 rgn->getOptionalIntAttribute(r.secondary_region_code, u"secondary_region_code", 0, 0xFF) &&
                 rgn->getOptionalIntAttribute(r.tertiary_region_code, u"tertiary_region_code", 0, 0xFFFF);
            if (ok && !r.country_code.has_value() && !r.primary_region_code.has_value()) {
                rgn->report().error(u"country_code and/or primary_region_code must be present in <%s>, line %d", {rgn->name(), rgn->lineNumber()});
                ok = false;
            }
            if (ok && !r.primary_region_code.has_value() && r.secondary_region_code.has_value()) {
                rgn->report().error(u"secondary_region_code cannot be used without primary_region_code in <%s>, line %d", {rgn->name(), rgn->lineNumber()});
                ok = false;
            }
            if (ok && !r.secondary_region_code.has_value() && r.tertiary_region_code.has_value()) {
                rgn->report().error(u"tertiary_region_code cannot be used without secondary_region_code in <%s>, line %d", {rgn->name(), rgn->lineNumber()});
                ok = false;
            }
            s.regions.push_back(r);
        }
        SOGI_list.push_back(s);
    }
    return ok;
}
