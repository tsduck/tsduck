//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCPCMDeliverySignallingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsTime.h"

#define MY_XML_NAME u"cpcm_delivery_signalling_descriptor"
#define MY_CLASS ts::CPCMDeliverySignallingDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_CPCM_DELIVERY_SIG
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CPCMDeliverySignallingDescriptor::CPCMDeliverySignallingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::CPCMDeliverySignallingDescriptor::clearContent()
{
    cpcm_version = 0;
    cpcm_v1_delivery_signalling.clearContent();
}

ts::CPCMDeliverySignallingDescriptor::CPCMDeliverySignallingDescriptor(DuckContext& duck, const Descriptor& desc) :
    CPCMDeliverySignallingDescriptor()
{
    deserialize(duck, desc);
}

void ts::CPCMDeliverySignallingDescriptor::CPCMv1Signalling::clearContent()
{
    copy_control = 0;
    do_not_cpcm_scramble = false;
    viewable = false;
    move_local = false;
    view_local = false;
    move_and_copy_propagation_information = 0;
    view_propagation_information = 0;
    remote_access_record_flag = false;
    export_beyond_trust = false;
    disable_analogue_sd_export = false;
    disable_analogue_sd_consumption = false;
    disable_analogue_hd_export = false;
    disable_analogue_hd_consumption = false;
    image_constraint = false;
    view_window_start.reset();
    view_window_end.reset();
    view_period_from_first_playback.reset();
    simultaneous_view_count.reset();
    remote_access_delay.reset();
    remote_access_date.reset();
    cps_vector.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::CPCMDeliverySignallingDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CPCMDeliverySignallingDescriptor::CPCMv1Signalling::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(copy_control, 3);
    buf.putBit(do_not_cpcm_scramble);
    buf.putBit(viewable);
    buf.putBit(view_window_start.has_value() || view_window_end.has_value());
    buf.putBit(view_period_from_first_playback.has_value());
    buf.putBit(simultaneous_view_count.has_value());
    buf.putBit(move_local);
    buf.putBit(view_local);
    buf.putBits(move_and_copy_propagation_information, 2);
    buf.putBits(view_propagation_information, 2);
    buf.putBit(remote_access_delay.has_value());
    buf.putBit(remote_access_date.has_value());
    buf.putBit(remote_access_record_flag);
    buf.putBit(!cps_vector.empty());
    buf.putBit(export_beyond_trust);
    buf.putBit(disable_analogue_sd_export);
    buf.putBit(disable_analogue_sd_consumption);
    buf.putBit(disable_analogue_hd_export);
    buf.putBit(disable_analogue_hd_consumption);
    buf.putBit(image_constraint);
    if (view_window_start.has_value() || view_window_end.has_value()) {
        buf.putMJD(view_window_start.value(), 5);
        buf.putMJD(view_window_end.value(), 5);
    }
    if (view_period_from_first_playback.has_value()) {
        buf.putUInt16(view_period_from_first_playback.value());
    }
    if (simultaneous_view_count.has_value()) {
        buf.putUInt8(simultaneous_view_count.value());
    }
    if (remote_access_delay.has_value()) {
        buf.putUInt16(remote_access_delay.value());
    }
    if (remote_access_date.has_value()) {
        buf.putMJD(remote_access_date.value(), 5);
    }
    if (!cps_vector.empty()) {
        buf.putBits(cps_vector.size(), 8);
        for (auto it : cps_vector) {
            buf.putUInt8(it.C_and_R_regime_mask);
            buf.putBits(it.cps_byte.size(), 16);
            buf.putBytes(it.cps_byte);
        }
    }
}

void ts::CPCMDeliverySignallingDescriptor::serializePayload(PSIBuffer& buf)  const
{
    buf.putUInt8(cpcm_version);
    switch (cpcm_version) {
        case 0x01:
            cpcm_v1_delivery_signalling.serializePayload(buf);
            break;
        default:
            break;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CPCMDeliverySignallingDescriptor::CPCMv1Signalling::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(copy_control, 3);
    do_not_cpcm_scramble = buf.getBool();
    viewable = buf.getBool();
    bool view_window_activated = buf.getBool();
    bool view_period_activated = buf.getBool();
    bool simultaneous_view_count_activated = buf.getBool();
    move_local = buf.getBool();
    view_local = buf.getBool();
    buf.getBits(move_and_copy_propagation_information, 2);
    buf.getBits(view_propagation_information, 2);
    bool remote_access_delay_flag = buf.getBool();
    bool remote_access_date_flag = buf.getBool();
    remote_access_record_flag = buf.getBool();
    bool export_controlled_cps = buf.getBool();
    export_beyond_trust = buf.getBool();
    disable_analogue_sd_export = buf.getBool();
    disable_analogue_sd_consumption = buf.getBool();
    disable_analogue_hd_export = buf.getBool();
    disable_analogue_hd_consumption = buf.getBool();
    image_constraint = buf.getBool();
    if (view_window_activated) {
        view_window_start = buf.getMJD(5);
        view_window_end = buf.getMJD(5);
    }
    if (view_period_activated) {
        view_period_from_first_playback = buf.getUInt16();
    }
    if (simultaneous_view_count_activated) {
        simultaneous_view_count = buf.getUInt8();
    }
    if (remote_access_delay_flag) {
        remote_access_delay = buf.getUInt16();
    }
    if (remote_access_date_flag) {
        remote_access_date = buf.getMJD(5);
    }
    if (export_controlled_cps) {
        uint8_t cps_vector_count = buf.getUInt8();
        for (uint8_t i = 0; i < cps_vector_count; i++) {
            CPSvector vec;
            vec.C_and_R_regime_mask = buf.getUInt8();
            uint16_t cps_vector_length = buf.getUInt16();
            vec.cps_byte = buf.getBytes(cps_vector_length);
            cps_vector.push_back(vec);
        }
    }
}

void ts::CPCMDeliverySignallingDescriptor::deserializePayload(PSIBuffer& buf)
{
    cpcm_version = buf.getUInt8();
    switch (cpcm_version) {
        case 0x01:
            cpcm_v1_delivery_signalling.deserializePayload(buf);
            break;
        default:
            cpcm_v1_delivery_signalling.clearContent();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

#define BYTE_TO_BINARY_PATTERN u"%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

void ts::CPCMDeliverySignallingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        uint8_t cpcm_version = buf.getUInt8();
        disp << margin << "CPCM version: " << int(cpcm_version) << std::endl;
        if (cpcm_version == 0x01) {
            disp << margin << "Copy control: " << DataName(MY_XML_NAME, u"copy_control", buf.getBits<uint8_t>(3), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
            disp << margin << "Do not CPCM scramble: " << UString::TrueFalse(buf.getBool());
            disp << ", viewable: " << UString::TrueFalse(buf.getBool()) << std::endl;
            bool view_window_activated = buf.getBool();
            bool view_period_activated = buf.getBool();
            bool simultaneous_view_count_activated = buf.getBool();
            disp << margin << "Move local: " << UString::TrueFalse(buf.getBool());
            disp << ", copy local: " << UString::TrueFalse(buf.getBool()) << std::endl;
            disp << margin << "Move and copy propagation: " << DataName(MY_XML_NAME, u"move_copy_propagation", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
            disp << margin << "View propagation: " << DataName(MY_XML_NAME, u"view_propagation", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
            bool remote_access_delay_flag = buf.getBool();
            bool remote_access_date_flag = buf.getBool();
            bool remote_access_record_flag = buf.getBool();
            bool export_controlled_cps = buf.getBool();
            bool export_beyond_trust = buf.getBool();
            bool sd_export = buf.getBool();
            bool sd_consume = buf.getBool();
            bool hd_export = buf.getBool();
            bool hd_consume = buf.getBool();
            bool image_constraint = buf.getBool();
            disp << margin << "Remote access record: " << UString::TrueFalse(remote_access_record_flag)
                << ", export beyond trust: " << UString::TrueFalse(export_beyond_trust)
                << ", image constraint : " << UString::TrueFalse(image_constraint) << std::endl;
            disp << margin << "Disable Analogue SD  export: " << UString::TrueFalse(sd_export) << ", consumption: " << UString::TrueFalse(sd_consume) << std::endl;
            disp << margin << "Disable Analogue HD  export: " << UString::TrueFalse(hd_export) << ", consumption: " << UString::TrueFalse(hd_consume) << std::endl;
            if (view_window_activated) {
                disp << margin << "View window start: " << buf.getMJD(5).format(ts::Time::FieldMask::DATETIME);
                disp << ", end: " << buf.getMJD(5).format(ts::Time::FieldMask::DATETIME) << std::endl;
            }
            if (view_period_activated) {
                disp << margin << "View period: " << buf.getUInt16() << " (15 minute periods)" << std::endl;
            }
            if (simultaneous_view_count_activated) {
                disp << margin << "Simultaneous view count " << int(buf.getUInt8()) << std::endl;
            }
            if (remote_access_delay_flag) {
                disp << margin << "Remote access delay: " << buf.getUInt16() << " (15 minute periods)" << std::endl;
            }
            if (remote_access_date_flag) {
                disp << margin << "Remote access date: " << buf.getMJD(5).format(ts::Time::FieldMask::DATETIME) << std::endl;
            }
            if (export_controlled_cps) {
                uint8_t cps_vector_count = buf.getUInt8();
                for (uint8_t i = 0; i < cps_vector_count; i++) {
                    uint8_t CandRmask = buf.getUInt8();
                    disp << margin << UString::Format(u"cps[%03d] - C and R regime mask: ", { i }) << UString::Format(BYTE_TO_BINARY_PATTERN, { BYTE_TO_BINARY(CandRmask) }) << std::endl;
                    uint16_t cps_vector_length = buf.getUInt16();
                    disp << margin << margin << UString::Dump(buf.getBytes(cps_vector_length), UString::SINGLE_LINE) << std::endl;
                }
            }
        }
        else
            disp << margin << " !! unsupported cpcm_version (" << int(cpcm_version) << ")" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CPCMDeliverySignallingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"cpcm_version", cpcm_version);
    if (cpcm_version == 0x01) {
        ts::xml::Element* v1 = root->addElement(u"cpcm_v1_delivery_signalling");
        v1->setIntAttribute(u"copy_control", cpcm_v1_delivery_signalling.copy_control);
        v1->setBoolAttribute(u"do_not_cpcm_scramble", cpcm_v1_delivery_signalling.do_not_cpcm_scramble);
        v1->setBoolAttribute(u"viewable", cpcm_v1_delivery_signalling.viewable);
        v1->setBoolAttribute(u"move_local", cpcm_v1_delivery_signalling.move_local);
        v1->setIntAttribute(u"move_and_copy_propagation_information", cpcm_v1_delivery_signalling.move_and_copy_propagation_information);
        v1->setIntAttribute(u"view_propagation_information", cpcm_v1_delivery_signalling.view_propagation_information);
        v1->setBoolAttribute(u"remote_access_record_flag", cpcm_v1_delivery_signalling.remote_access_record_flag);
        v1->setBoolAttribute(u"export_beyond_trust", cpcm_v1_delivery_signalling.export_beyond_trust);
        v1->setBoolAttribute(u"disable_analogue_sd_export", cpcm_v1_delivery_signalling.disable_analogue_sd_export);
        v1->setBoolAttribute(u"disable_analogue_sd_consumption", cpcm_v1_delivery_signalling.disable_analogue_sd_consumption);
        v1->setBoolAttribute(u"disable_analogue_hd_export", cpcm_v1_delivery_signalling.disable_analogue_hd_export);
        v1->setBoolAttribute(u"disable_analogue_hd_consumption", cpcm_v1_delivery_signalling.disable_analogue_hd_consumption);
        v1->setBoolAttribute(u"image_constraint", cpcm_v1_delivery_signalling.image_constraint);

        if (cpcm_v1_delivery_signalling.view_window_start.has_value()) {
            v1->setDateTimeAttribute(u"view_window_start", cpcm_v1_delivery_signalling.view_window_start.value());
        }
        if (cpcm_v1_delivery_signalling.view_window_end.has_value()) {
            v1->setDateTimeAttribute(u"view_window_end", cpcm_v1_delivery_signalling.view_window_end.value());
        }
        v1->setOptionalIntAttribute(u"view_period_from_first_playback", cpcm_v1_delivery_signalling.view_period_from_first_playback);
        v1->setOptionalIntAttribute(u"simultaneous_view_count", cpcm_v1_delivery_signalling.simultaneous_view_count);
        v1->setOptionalIntAttribute(u"remote_access_delay", cpcm_v1_delivery_signalling.remote_access_delay);
        if (cpcm_v1_delivery_signalling.remote_access_date.has_value()) {
            v1->setDateTimeAttribute(u"remote_access_date", cpcm_v1_delivery_signalling.remote_access_date.value());
        }
        for (auto it : cpcm_v1_delivery_signalling.cps_vector) {
            ts::xml::Element* cps = v1->addElement(u"cps");
            cps->setIntAttribute(u"C_and_R_regime_mask", it.C_and_R_regime_mask);
            cps->addHexaText(it.cps_byte);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CPCMDeliverySignallingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(cpcm_version, u"cpcm_version", true, 0, 0, 0x01);
    if (cpcm_version == 0x01) {
        xml::ElementVector children;
        ok &= element->getChildren(children, u"cpcm_v1_delivery_signalling", 0, 1);
        for (size_t i = 0; ok && i < children.size(); ++i) {
            xml::ElementVector cpsList;
            ok = children[i]->getIntAttribute(cpcm_v1_delivery_signalling.copy_control, u"copy_control", true, 0, 0, 7) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.do_not_cpcm_scramble, u"do_not_cpcm_scramble", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.viewable, u"viewable", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.move_local, u"move_local", true) &&
                children[i]->getIntAttribute(cpcm_v1_delivery_signalling.move_and_copy_propagation_information, u"move_and_copy_propagation_information", true, 0, 0, 3) &&
                children[i]->getIntAttribute(cpcm_v1_delivery_signalling.view_propagation_information, u"view_propagation_information", true, 0, 0, 3) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.remote_access_record_flag, u"remote_access_record_flag", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.export_beyond_trust, u"export_beyond_trust", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.disable_analogue_sd_export, u"disable_analogue_sd_export", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.disable_analogue_sd_consumption, u"disable_analogue_sd_consumption", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.disable_analogue_hd_export, u"disable_analogue_hd_export", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.disable_analogue_hd_consumption, u"disable_analogue_hd_consumption", true) &&
                children[i]->getBoolAttribute(cpcm_v1_delivery_signalling.image_constraint, u"image_constraint", true) &&
                children[i]->getOptionalIntAttribute(cpcm_v1_delivery_signalling.view_period_from_first_playback, u"view_period_from_first_playback") &&
                children[i]->getOptionalIntAttribute(cpcm_v1_delivery_signalling.simultaneous_view_count, u"simultaneous_view_count") &&
                children[i]->getOptionalIntAttribute(cpcm_v1_delivery_signalling.remote_access_delay, u"remote_access_delay") &&
                children[i]->getChildren(cpsList, u"cps", 0, 0xFF);

            Time tmpDate;
            if (ok && children[i]->hasAttribute(u"view_window_start")) {
                ok = children[i]->getDateTimeAttribute(tmpDate, u"view_window_start", true);
                cpcm_v1_delivery_signalling.view_window_start = tmpDate;
            }
            if (ok && children[i]->hasAttribute(u"view_window_end")) {
                ok = children[i]->getDateTimeAttribute(tmpDate, u"view_window_end", true);
                cpcm_v1_delivery_signalling.view_window_end = tmpDate;
            }
            if (ok && children[i]->hasAttribute(u"remote_access_date")) {
                ok = children[i]->getDateTimeAttribute(tmpDate, u"remote_access_date", true);
                cpcm_v1_delivery_signalling.remote_access_date = tmpDate;
            }
            for (size_t j = 0; ok && j < cpsList.size(); ++j) {
                CPSvector newCPS;
                ok &= cpsList[j]->getIntAttribute(newCPS.C_and_R_regime_mask, u"C_and_R_regime_mask") &&
                    cpsList[j]->getHexaText(newCPS.cps_byte);
                cpcm_v1_delivery_signalling.cps_vector.push_back(newCPS);
            }
         }
    }
    return ok;

}
