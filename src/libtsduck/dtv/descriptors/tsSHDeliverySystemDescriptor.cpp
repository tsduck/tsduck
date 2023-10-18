//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSHDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SH_delivery_system_descriptor"
#define MY_CLASS ts::SHDeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_SH_DELIVERY
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::SHDeliverySystemDescriptor::SHDeliverySystemDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SHDeliverySystemDescriptor::SHDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    SHDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::SHDeliverySystemDescriptor::clearContent()
{
    diversity_mode = 0;
    modulations.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::SHDeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(diversity_mode, 4);
    buf.putBits(0xFF, 4);
    for (const auto& it : modulations) {
        buf.putBit(it.is_ofdm);
        buf.putBit(it.interleaver_presence);
        buf.putBit(it.short_interleaver);
        buf.putBits(0xFF, 5);
        if (it.is_ofdm) {
            buf.putBits(it.ofdm.bandwidth, 3);
            buf.putBit(it.ofdm.priority);
            buf.putBits(it.ofdm.constellation_and_hierarchy, 3);
            buf.putBits(it.ofdm.code_rate, 4);
            buf.putBits(it.ofdm.guard_interval, 2);
            buf.putBits(it.ofdm.transmission_mode, 2);
            buf.putBit(it.ofdm.common_frequency);
        }
        else {
            buf.putBits(it.tdm.polarization, 2);
            buf.putBits(it.tdm.roll_off, 2);
            buf.putBits(it.tdm.modulation_mode, 2);
            buf.putBits(it.tdm.code_rate, 4);
            buf.putBits(it.tdm.symbol_rate, 5);
            buf.putBit(1);
        }
        if (it.interleaver_presence) {
            buf.putBits(it.common_multiplier, 6);
            if (it.short_interleaver) {
                buf.putBits(0xFF, 2);
            }
            else {
                buf.putBits(it.nof_late_taps, 6);
                buf.putBits(it.nof_slices, 6);
                buf.putBits(it.slice_distance, 8);
                buf.putBits(it.non_late_increments, 6);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(diversity_mode, 4);
    buf.skipBits(4);
    while (buf.canRead()) {
        Modulation mod;
        mod.is_ofdm = buf.getBool();
        mod.interleaver_presence = buf.getBool();
        mod.short_interleaver = buf.getBool();
        buf.skipBits(5);
        if (mod.is_ofdm) {
            buf.getBits(mod.ofdm.bandwidth, 3);
            mod.ofdm.priority = buf.getBit();
            buf.getBits(mod.ofdm.constellation_and_hierarchy, 3);
            buf.getBits(mod.ofdm.code_rate, 4);
            buf.getBits(mod.ofdm.guard_interval, 2);
            buf.getBits(mod.ofdm.transmission_mode, 2);
            mod.ofdm.common_frequency = buf.getBool();
        }
        else {
            buf.getBits(mod.tdm.polarization, 2);
            buf.getBits(mod.tdm.roll_off, 2);
            buf.getBits(mod.tdm.modulation_mode, 2);
            buf.getBits(mod.tdm.code_rate, 4);
            buf.getBits(mod.tdm.symbol_rate, 5);
            buf.skipBits(1);
        }
        if (mod.interleaver_presence) {
            buf.getBits(mod.common_multiplier, 6);
            if (mod.short_interleaver) {
                buf.skipBits(2);
            }
            else {
                buf.getBits(mod.nof_late_taps, 6);
                buf.getBits(mod.nof_slices, 6);
                buf.getBits(mod.slice_distance, 8);
                buf.getBits(mod.non_late_increments, 6);
            }
        }
        modulations.push_back(mod);
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML and disp.
//----------------------------------------------------------------------------

const ts::Enumeration ts::SHDeliverySystemDescriptor::BandwidthNames({
    {u"8MHz",   0},
    {u"7MHz",   1},
    {u"6MHz",   2},
    {u"5MHz",   3},
    {u"1.7MHz", 4},
});

const ts::Enumeration ts::SHDeliverySystemDescriptor::GuardIntervalNames({
    {u"1/32", 0},
    {u"1/16", 1},
    {u"1/8",  2},
    {u"1/4",  3},
});

const ts::Enumeration ts::SHDeliverySystemDescriptor::TransmissionModeNames({
    {u"1k",  0},
    {u"2k",  1},
    {u"4k",  2},
    {u"8k",  3},
});

const ts::Enumeration ts::SHDeliverySystemDescriptor::PolarizationNames({
    {u"horizontal", 0},
    {u"vertical",   1},
    {u"left",       2},
    {u"right",      3},
});

const ts::Enumeration ts::SHDeliverySystemDescriptor::RollOffNames({
    {u"0.35",     0},
    {u"0.25",     1},
    {u"0.15",     2},
    {u"reserved", 3},
});

const ts::Enumeration ts::SHDeliverySystemDescriptor::ModulationNames({
    {u"QPSK",     0},
    {u"8PSK",     1},
    {u"16APSK",   2},
    {u"reserved", 3},
});



//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const uint8_t div = buf.getBits<uint8_t>(4);
        buf.skipBits(4);
        disp << margin << UString::Format(u"Diversity mode: 0x%X", {div});
        if ((div & 0x08) != 0) {
            disp << ", paTS";
        }
        if ((div & 0x04) != 0) {
            disp << ", FEC diversity";
        }
        if ((div & 0x02) != 0) {
            disp << ", FEC at phy";
        }
        if ((div & 0x01) != 0) {
            disp << ", FEC at link";
        }
        disp << std::endl;

        while (buf.canReadBytes(3)) {
            const bool is_ofdm = buf.getBool();
            const bool interleaver = buf.getBool();
            const bool short_interleaver = buf.getBool();
            buf.skipBits(5);
            if (is_ofdm) {
                disp << margin << "- Modulation type: OFDM" << std::endl;
                disp << margin << "  Bandwidth: " << BandwidthNames.name(buf.getBits<uint8_t>(3)) << std::endl;
                disp << margin << UString::Format(u"  Priority: %d", {buf.getBit()}) << std::endl;
                disp << margin << "  Constellation & hierarchy: " << DataName(MY_XML_NAME, u"ConstellationHierarchy", buf.getBits<uint8_t>(3), NamesFlags::FIRST) << std::endl;
                disp << margin << "  Code rate: " << DataName(MY_XML_NAME, u"CodeRate", buf.getBits<uint8_t>(4), NamesFlags::FIRST) << std::endl;
                disp << margin << "  Guard interval: " << GuardIntervalNames.name(buf.getBits<uint8_t>(2)) << std::endl;
                disp << margin << "  Transmission mode: " << TransmissionModeNames.name(buf.getBits<uint8_t>(2)) << std::endl;
                disp << margin << UString::Format(u"  Common frequency: %s", {buf.getBool()}) << std::endl;
            }
            else {
                disp << margin << "- Modulation type: TDM" << std::endl;
                disp << margin << "  Polarization: " << PolarizationNames.name(buf.getBits<uint8_t>(2)) << std::endl;
                disp << margin << "  Roll off: " << RollOffNames.name(buf.getBits<uint8_t>(2)) << std::endl;
                disp << margin << "  Modulation mode: " << ModulationNames.name(buf.getBits<uint8_t>(2)) << std::endl;
                disp << margin << "  Code rate: " << DataName(MY_XML_NAME, u"CodeRate", buf.getBits<uint8_t>(4), NamesFlags::FIRST) << std::endl;
                disp << margin << UString::Format(u"  Symbol rate code: 0x%X (%<d)", {buf.getBits<uint8_t>(5)}) << std::endl;
                buf.skipBits(1);
            }
            if (interleaver && buf.canReadBytes(short_interleaver ? 1 : 4)) {
                disp << margin << UString::Format(u"  Common multiplier: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
                if (short_interleaver) {
                    buf.skipBits(2);
                }
                else {
                    disp << margin << UString::Format(u"  Number of late taps: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
                    disp << margin << UString::Format(u"  Number of slices: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
                    disp << margin << UString::Format(u"  Slice distance: %d", {buf.getBits<uint8_t>(8)}) << std::endl;
                    disp << margin << UString::Format(u"  Non-late increments: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"diversity_mode", diversity_mode, true);
    for (const auto& it : modulations) {
        xml::Element* mod = root->addElement(u"modulation");
        if (it.is_ofdm) {
            xml::Element* e = mod->addElement(u"OFDM");
            e->setIntEnumAttribute(BandwidthNames, u"bandwidth", it.ofdm.bandwidth);
            e->setIntAttribute(u"priority", it.ofdm.priority);
            e->setIntAttribute(u"constellation_and_hierarchy", it.ofdm.constellation_and_hierarchy);
            e->setIntAttribute(u"code_rate", it.ofdm.code_rate);
            e->setIntEnumAttribute(GuardIntervalNames, u"guard_interval", it.ofdm.guard_interval);
            e->setIntEnumAttribute(TransmissionModeNames, u"transmission_mode", it.ofdm.transmission_mode);
            e->setBoolAttribute(u"common_frequency", it.ofdm.common_frequency);
        }
        else {
            xml::Element* e = mod->addElement(u"TDM");
            e->setIntEnumAttribute(PolarizationNames, u"polarization", it.tdm.polarization);
            e->setIntEnumAttribute(RollOffNames, u"roll_off", it.tdm.roll_off);
            e->setIntEnumAttribute(ModulationNames, u"modulation_mode", it.tdm.modulation_mode);
            e->setIntAttribute(u"code_rate", it.tdm.code_rate);
            e->setIntAttribute(u"symbol_rate", it.tdm.symbol_rate);
        }
        if (it.interleaver_presence) {
            xml::Element* e = mod->addElement(u"interleaver");
            e->setIntAttribute(u"common_multiplier", it.common_multiplier);
            if (!it.short_interleaver) {
                e->setIntAttribute(u"nof_late_taps", it.nof_late_taps);
                e->setIntAttribute(u"nof_slices", it.nof_slices);
                e->setIntAttribute(u"slice_distance", it.slice_distance);
                e->setIntAttribute(u"non_late_increments", it.non_late_increments);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SHDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xmods;
    bool ok =
        element->getIntAttribute(diversity_mode, u"diversity_mode", true, 0, 0, 15) &&
        element->getChildren(xmods, u"modulation");

    for (size_t i = 0; ok && i < xmods.size(); ++i) {
        Modulation mod;
        xml::ElementVector xofdm;
        xml::ElementVector xtdm;
        xml::ElementVector xint;
        ok = xmods[i]->getChildren(xofdm, u"OFDM", 0, 1) &&
             xmods[i]->getChildren(xtdm, u"TDM", xofdm.empty() ? 1 : 0, xofdm.empty() ? 1 : 0) &&
             xmods[i]->getChildren(xint, u"interleaver", 0, 1);

        if (ok) {
            mod.is_ofdm = !xofdm.empty();
            if (mod.is_ofdm) {
                assert(xofdm.size() == 1);
                ok = xofdm[0]->getIntEnumAttribute(mod.ofdm.bandwidth, BandwidthNames, u"bandwidth", true) &&
                     xofdm[0]->getIntAttribute(mod.ofdm.priority, u"priority", true, 0, 0, 1) &&
                     xofdm[0]->getIntAttribute(mod.ofdm.constellation_and_hierarchy, u"constellation_and_hierarchy", true, 0, 0, 0x07) &&
                     xofdm[0]->getIntAttribute(mod.ofdm.code_rate, u"code_rate", true, 0, 0, 0x0F) &&
                     xofdm[0]->getIntEnumAttribute(mod.ofdm.guard_interval, GuardIntervalNames, u"guard_interval", true) &&
                     xofdm[0]->getIntEnumAttribute(mod.ofdm.transmission_mode, TransmissionModeNames, u"transmission_mode", true) &&
                     xofdm[0]->getBoolAttribute(mod.ofdm.common_frequency, u"common_frequency", true);
            }
            else {
                assert(xtdm.size() == 1);
                ok = xtdm[0]->getIntEnumAttribute(mod.tdm.polarization, PolarizationNames, u"polarization", true) &&
                     xtdm[0]->getIntEnumAttribute(mod.tdm.roll_off, RollOffNames, u"roll_off", true) &&
                     xtdm[0]->getIntEnumAttribute(mod.tdm.modulation_mode, ModulationNames, u"modulation_mode", true) &&
                     xtdm[0]->getIntAttribute(mod.tdm.code_rate, u"code_rate", true, 0, 0, 0x0F) &&
                     xtdm[0]->getIntAttribute(mod.tdm.symbol_rate, u"symbol_rate", true, 0, 0, 0x1F);
            }
        }

        mod.interleaver_presence = ok && !xint.empty();
        if (mod.interleaver_presence) {
            assert(xint.size() == 1);
            ok = xint[0]->getIntAttribute(mod.common_multiplier, u"common_multiplier", true, 0, 0, 0x3F);
            if (ok) {
                const int attr_count =
                    xint[0]->hasAttribute(u"nof_late_taps") +
                    xint[0]->hasAttribute(u"nof_slices") +
                    xint[0]->hasAttribute(u"slice_distance") +
                    xint[0]->hasAttribute(u"non_late_increments");
                mod.short_interleaver = attr_count == 0;
                if (attr_count == 4) {
                    ok = xint[0]->getIntAttribute(mod.nof_late_taps, u"nof_late_taps", true, 0, 0, 0x3F) &&
                         xint[0]->getIntAttribute(mod.nof_slices, u"nof_slices", true, 0, 0, 0x3F) &&
                         xint[0]->getIntAttribute(mod.slice_distance, u"slice_distance", true, 0, 0, 0xFF) &&
                         xint[0]->getIntAttribute(mod.non_late_increments, u"non_late_increments", true, 0, 0, 0x3F);
                }
                else if (attr_count != 0) {
                    ok = false;
                    element->report().error(u"in <%s>, line %d, attributes nof_late_taps, nof_slices, slice_distance, "
                                            u"non_late_increments must be all present or all absent",
                                            {xint[0]->name(), xint[0]->lineNumber()});
                }
            }
        }

        modulations.push_back(mod);
    }
    return ok;
}
