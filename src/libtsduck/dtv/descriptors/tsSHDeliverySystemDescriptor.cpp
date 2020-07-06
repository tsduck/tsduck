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

#include "tsSHDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    diversity_mode(0),
    modulations()
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

ts::SHDeliverySystemDescriptor::Modulation::Modulation() :
    is_ofdm(false),
    tdm(),
    ofdm(),
    interleaver_presence(false),
    short_interleaver(false),
    common_multiplier(0),
    nof_late_taps(0),
    nof_slices(0),
    slice_distance(0),
    non_late_increments(0)
{
}

ts::SHDeliverySystemDescriptor::TDM::TDM() :
    polarization(0),
    roll_off(0),
    modulation_mode(0),
    code_rate(0),
    symbol_rate(0)
{
}

ts::SHDeliverySystemDescriptor::OFDM::OFDM() :
    bandwidth(0),
    priority(0),
    constellation_and_hierarchy(0),
    code_rate(0),
    guard_interval(0),
    transmission_mode(0),
    common_frequency(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8(uint8_t((diversity_mode & 0x0F) << 4) | 0x0F);
    for (auto it = modulations.begin(); it != modulations.end(); ++it) {
        bbp->appendUInt8((it->is_ofdm ? 0x80 : 0x00) |
                         (it->interleaver_presence ? 0x40 : 0x00) |
                         (it->short_interleaver ? 0x20 : 0x00) |
                         0x1F);
        if (it->is_ofdm) {
            bbp->appendUInt16(uint16_t((it->ofdm.bandwidth & 0x07) << 13) |
                              uint16_t((it->ofdm.priority & 0x01) << 12) |
                              uint16_t((it->ofdm.constellation_and_hierarchy & 0x07) << 9) |
                              uint16_t((it->ofdm.code_rate & 0x0F) << 5) |
                              uint16_t((it->ofdm.guard_interval & 0x03) << 3) |
                              uint16_t((it->ofdm.transmission_mode & 0x03) << 1) |
                              (it->ofdm.common_frequency ? 0x0001 : 0x0000));
        }
        else {
            bbp->appendUInt16(uint16_t((it->tdm.polarization & 0x03) << 14) |
                              uint16_t((it->tdm.roll_off & 0x03) << 12) |
                              uint16_t((it->tdm.modulation_mode & 0x03) << 10) |
                              uint16_t((it->tdm.code_rate & 0x0F) << 6) |
                              uint16_t((it->tdm.symbol_rate & 0x1F) << 1) |
                              0x0001);
        }
        if (it->interleaver_presence) {
            if (it->short_interleaver) {
                bbp->appendUInt8(uint8_t((it->common_multiplier & 0x3F) << 2) | 0x03);
            }
            else {
                bbp->appendUInt32(uint32_t((it->common_multiplier & 0x3F) << 26) |
                                  uint32_t((it->nof_late_taps & 0x3F) << 20) |
                                  uint32_t((it->nof_slices & 0x3F) << 14) |
                                  uint32_t(it->slice_distance << 6) |
                                  uint32_t(it->non_late_increments & 0x3F));
            }
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    modulations.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2 && data[0] == MY_EDID;

    if (_is_valid) {
        diversity_mode = (data[1] >> 4) & 0x0F;
        data += 2; size -= 2;
    }
    while (_is_valid && size >= 3) {
        Modulation mod;
        const uint8_t flags = data[0];
        const uint16_t mval = GetUInt16(data + 1);
        data += 3; size -= 3;

        mod.is_ofdm = (flags & 0x80) != 0;
        mod.interleaver_presence = (flags & 0x40) != 0;
        mod.short_interleaver = (flags & 0x20) != 0;

        if (mod.is_ofdm) {
            mod.ofdm.bandwidth = uint8_t(mval >> 13) & 0x07;
            mod.ofdm.priority = uint8_t(mval >> 12) & 0x01;
            mod.ofdm.constellation_and_hierarchy = uint8_t(mval >> 9) & 0x07;
            mod.ofdm.code_rate = uint8_t(mval >> 5) & 0x0F;
            mod.ofdm.guard_interval = uint8_t(mval >> 3) & 0x03;
            mod.ofdm.transmission_mode = uint8_t(mval >> 1) & 0x03;
            mod.ofdm.common_frequency = (mval & 0x0001) != 0;
        }
        else {
            mod.tdm.polarization = uint8_t(mval >> 14) & 0x03;
            mod.tdm.roll_off = uint8_t(mval >> 12) & 0x03;
            mod.tdm.modulation_mode = uint8_t(mval >> 10) & 0x03;
            mod.tdm.code_rate = uint8_t(mval >> 6) & 0x0F;
            mod.tdm.symbol_rate = uint8_t(mval >> 1) & 0x1F;
        }

        if (mod.interleaver_presence) {
            const size_t min_size = mod.short_interleaver ? 1 : 4;
            if (size < min_size) {
                _is_valid = false;
                break;
            }
            mod.common_multiplier = (data[0] >> 2) & 0x3F;
            if (!mod.short_interleaver) {
                const uint32_t val = GetUInt32(data);
                mod.nof_late_taps = uint8_t(val >> 20) & 0x3F;
                mod.nof_slices = uint8_t(val >> 14) & 0x3F;
                mod.slice_distance = uint8_t(val >> 6);
                mod.non_late_increments = uint8_t(val & 0x3F);
            }
            data += min_size; size -= min_size;
        }

        modulations.push_back(mod);
    }
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Enumerations for XML and display.
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

void ts::SHDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t div = (data[0] >> 4) & 0x0F;
        data++; size--;

        strm << margin << UString::Format(u"Diversity mode: 0x%X", {div});
        if ((div & 0x08) != 0) {
            strm << ", paTS";
        }
        if ((div & 0x04) != 0) {
            strm << ", FEC diversity";
        }
        if ((div & 0x02) != 0) {
            strm << ", FEC at phy";
        }
        if ((div & 0x01) != 0) {
            strm << ", FEC at link";
        }
        strm << std::endl;

        while (size >= 3) {
            const uint8_t flags = data[0];
            const uint16_t mval = GetUInt16(data + 1);
            data += 3; size -= 3;

            if ((flags & 0x80) != 0) {
                strm << margin << "- Modulation type: OFDM" << std::endl
                     << margin << UString::Format(u"  Bandwidth: %s", {BandwidthNames.name((mval >> 13) & 0x07)}) << std::endl
                     << margin << UString::Format(u"  Priority: %d", {(mval >> 12) & 0x01}) << std::endl
                     << margin << UString::Format(u"  Constellation & hierarchy: %s", {NameFromSection(u"SHConstellationHierarchy", (mval >> 9) & 0x07, names::FIRST)}) << std::endl
                     << margin << UString::Format(u"  Code rate: %s", {NameFromSection(u"SHCodeRate", (mval >> 5) & 0x0F, names::FIRST)}) << std::endl
                     << margin << UString::Format(u"  Guard interval: %s", {GuardIntervalNames.name((mval >> 3) & 0x03)}) << std::endl
                     << margin << UString::Format(u"  Transmission mode: %s", {TransmissionModeNames.name((mval >> 1) & 0x03)}) << std::endl
                     << margin << UString::Format(u"  Common frequency: %s", {(mval & 0x01) != 0}) << std::endl;
            }
            else {
                const uint8_t symrate = uint8_t(mval >> 1) & 0x1F;
                strm << margin << "- Modulation type: TDM" << std::endl
                     << margin << UString::Format(u"  Polarization: %s", {PolarizationNames.name((mval >> 14) & 0x03)}) << std::endl
                     << margin << UString::Format(u"  Roll off: %s", {RollOffNames.name((mval >> 12) & 0x03)}) << std::endl
                     << margin << UString::Format(u"  Modulation mode: %s", {ModulationNames.name((mval >> 10) & 0x03)}) << std::endl
                     << margin << UString::Format(u"  Code rate: %s", {NameFromSection(u"SHCodeRate", (mval >> 6) & 0x0F, names::FIRST)}) << std::endl
                     << margin << UString::Format(u"  Symbol rate code: 0x%X (%d)", {symrate, symrate}) << std::endl;
            }

            if ((flags & 0x40) != 0) {
                const bool short_interleaver = (flags & 0x20) != 0;
                const size_t min_size = short_interleaver ? 1 : 4;
                if (size < min_size) {
                    break;
                }
                strm << margin << UString::Format(u"  Common multiplier: %d", {(data[0] >> 2) & 0x3F}) << std::endl;
                if (!short_interleaver) {
                    const uint32_t val = GetUInt32(data);
                    strm << margin << UString::Format(u"  Number of late taps: %d", {(val >> 20) & 0x3F}) << std::endl
                         << margin << UString::Format(u"  Number of slices: %d", {(val >> 14) & 0x3F}) << std::endl
                         << margin << UString::Format(u"  Slice distance: %d", {(val >> 6) & 0xFF}) << std::endl
                         << margin << UString::Format(u"  Non-late increments: %d", {val & 0x3F}) << std::endl;
                }
                data += min_size; size -= min_size;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SHDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"diversity_mode", diversity_mode, true);
    for (auto it = modulations.begin(); it != modulations.end(); ++it) {
        xml::Element* mod = root->addElement(u"modulation");
        if (it->is_ofdm) {
            xml::Element* e = mod->addElement(u"OFDM");
            e->setIntEnumAttribute(BandwidthNames, u"bandwidth", it->ofdm.bandwidth);
            e->setIntAttribute(u"priority", it->ofdm.priority);
            e->setIntAttribute(u"constellation_and_hierarchy", it->ofdm.constellation_and_hierarchy);
            e->setIntAttribute(u"code_rate", it->ofdm.code_rate);
            e->setIntEnumAttribute(GuardIntervalNames, u"guard_interval", it->ofdm.guard_interval);
            e->setIntEnumAttribute(TransmissionModeNames, u"transmission_mode", it->ofdm.transmission_mode);
            e->setBoolAttribute(u"common_frequency", it->ofdm.common_frequency);
        }
        else {
            xml::Element* e = mod->addElement(u"TDM");
            e->setIntEnumAttribute(PolarizationNames, u"polarization", it->tdm.polarization);
            e->setIntEnumAttribute(RollOffNames, u"roll_off", it->tdm.roll_off);
            e->setIntEnumAttribute(ModulationNames, u"modulation_mode", it->tdm.modulation_mode);
            e->setIntAttribute(u"code_rate", it->tdm.code_rate);
            e->setIntAttribute(u"symbol_rate", it->tdm.symbol_rate);
        }
        if (it->interleaver_presence) {
            xml::Element* e = mod->addElement(u"interleaver");
            e->setIntAttribute(u"common_multiplier", it->common_multiplier);
            if (!it->short_interleaver) {
                e->setIntAttribute(u"nof_late_taps", it->nof_late_taps);
                e->setIntAttribute(u"nof_slices", it->nof_slices);
                e->setIntAttribute(u"slice_distance", it->slice_distance);
                e->setIntAttribute(u"non_late_increments", it->non_late_increments);
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
        element->getIntAttribute<uint8_t>(diversity_mode, u"diversity_mode", true, 0, 0, 15) &&
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
                     xofdm[0]->getIntAttribute<uint8_t>(mod.ofdm.priority, u"priority", true, 0, 0, 1) &&
                     xofdm[0]->getIntAttribute<uint8_t>(mod.ofdm.constellation_and_hierarchy, u"constellation_and_hierarchy", true, 0, 0, 0x07) &&
                     xofdm[0]->getIntAttribute<uint8_t>(mod.ofdm.code_rate, u"code_rate", true, 0, 0, 0x0F) &&
                     xofdm[0]->getIntEnumAttribute(mod.ofdm.guard_interval, GuardIntervalNames, u"guard_interval", true) &&
                     xofdm[0]->getIntEnumAttribute(mod.ofdm.transmission_mode, TransmissionModeNames, u"transmission_mode", true) &&
                     xofdm[0]->getBoolAttribute(mod.ofdm.common_frequency, u"common_frequency", true);
            }
            else {
                assert(xtdm.size() == 1);
                ok = xtdm[0]->getIntEnumAttribute(mod.tdm.polarization, PolarizationNames, u"polarization", true) &&
                     xtdm[0]->getIntEnumAttribute(mod.tdm.roll_off, RollOffNames, u"roll_off", true) &&
                     xtdm[0]->getIntEnumAttribute(mod.tdm.modulation_mode, ModulationNames, u"modulation_mode", true) &&
                     xtdm[0]->getIntAttribute<uint8_t>(mod.tdm.code_rate, u"code_rate", true, 0, 0, 0x0F) &&
                     xtdm[0]->getIntAttribute<uint8_t>(mod.tdm.symbol_rate, u"symbol_rate", true, 0, 0, 0x1F);
            }
        }

        mod.interleaver_presence = ok && !xint.empty();
        if (mod.interleaver_presence) {
            assert(xint.size() == 1);
            ok = xint[0]->getIntAttribute<uint8_t>(mod.common_multiplier, u"common_multiplier", true, 0, 0, 0x3F);
            if (ok) {
                const int attr_count =
                    xint[0]->hasAttribute(u"nof_late_taps") +
                    xint[0]->hasAttribute(u"nof_slices") +
                    xint[0]->hasAttribute(u"slice_distance") +
                    xint[0]->hasAttribute(u"non_late_increments");
                mod.short_interleaver = attr_count == 0;
                if (attr_count == 4) {
                    ok = xint[0]->getIntAttribute<uint8_t>(mod.nof_late_taps, u"nof_late_taps", true, 0, 0, 0x3F) &&
                         xint[0]->getIntAttribute<uint8_t>(mod.nof_slices, u"nof_slices", true, 0, 0, 0x3F) &&
                         xint[0]->getIntAttribute<uint8_t>(mod.slice_distance, u"slice_distance", true, 0, 0, 0xFF) &&
                         xint[0]->getIntAttribute<uint8_t>(mod.non_late_increments, u"non_late_increments", true, 0, 0, 0x3F);
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
