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

#include "tsT2DeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"T2_delivery_system_descriptor"
#define MY_CLASS ts::T2DeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_T2_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::T2DeliverySystemDescriptor::T2DeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_T2, MY_XML_NAME),
    plp_id(0),
    T2_system_id(0),
    has_extension(false),
    SISO_MISO(0),
    bandwidth(0),
    guard_interval(0),
    transmission_mode(0),
    other_frequency(false),
    tfs(false),
    cells()
{
}

ts::T2DeliverySystemDescriptor::T2DeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    T2DeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::T2DeliverySystemDescriptor::clearContent()
{
    plp_id = 0;
    T2_system_id = 0;
    has_extension = false;
    SISO_MISO = 0;
    bandwidth = 0;
    guard_interval = 0;
    transmission_mode = 0;
    other_frequency = false;
    tfs = false;
    cells.clear();
}

ts::T2DeliverySystemDescriptor::Cell::Cell() :
    cell_id(0),
    centre_frequency(),
    subcells()
{
}

ts::T2DeliverySystemDescriptor::Subcell::Subcell() :
    cell_id_extension(0),
    transposer_frequency(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::T2DeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8(plp_id);
    bbp->appendUInt16(T2_system_id);
    if (has_extension) {
        bbp->appendUInt8(uint8_t((SISO_MISO & 0x03) << 6) |
                         uint8_t((bandwidth & 0x0F) << 2) |
                         0x03);
        bbp->appendUInt8(uint8_t((guard_interval & 0x07) << 5) |
                         uint8_t((transmission_mode & 0x07) << 2) |
                         (other_frequency ? 0x02 : 0x00) |
                         (tfs ? 0x01 : 0x00));
        for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
            bbp->appendUInt16(it1->cell_id);
            if (tfs) {
                bbp->appendUInt8(uint8_t(4 * it1->centre_frequency.size()));
                for (auto it2 = it1->centre_frequency.begin(); it2 != it1->centre_frequency.end(); ++it2) {
                    bbp->appendUInt32(uint32_t(*it2 / 10)); // encoded in units of 10 Hz
                }
            }
            else {
                bbp->appendUInt32(uint32_t((it1->centre_frequency.empty() ? 0 : it1->centre_frequency.front()) / 10)); // encoded in units of 10 Hz
            }
            bbp->appendUInt8(uint8_t(5 * it1->subcells.size()));
            for (auto it2 = it1->subcells.begin(); it2 != it1->subcells.end(); ++it2) {
                bbp->appendUInt8(it2->cell_id_extension);
                bbp->appendUInt32(uint32_t(it2->transposer_frequency / 10)); // encoded in units of 10 Hz
            }
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::T2DeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    has_extension = false;
    cells.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4 && data[0] == MY_EDID;

    if (_is_valid) {
        plp_id = data[1];
        T2_system_id = GetUInt16(data + 2);
        data += 4; size -= 4;

        _is_valid = size == 0 || size >= 2;
        has_extension = size >= 2;

        if (has_extension) {
            SISO_MISO = (data[0] >> 6) & 0x03;
            bandwidth = (data[0] >> 2) & 0x0F;
            guard_interval = (data[1] >> 5) & 0x07;
            transmission_mode = (data[1] >> 2) & 0x07;
            other_frequency = (data[1] & 0x02) != 0;
            tfs = (data[1] & 0x01) != 0;
            data += 2; size -= 2;

            while (size >= 3) {
                Cell cell;
                cell.cell_id = GetUInt16(data);
                data += 2; size -= 2;

                if (tfs) {
                    size_t len = data[0];
                    data++; size--;
                    while (len >= 4 && size >= 4) {
                        cell.centre_frequency.push_back(uint64_t(GetUInt32(data)) * 10); // encoded unit is 10 Hz
                        data += 4; size -= 4; len -= 4;
                    }
                    if (len > 0) {
                        _is_valid = false;
                        return;
                    }
                }
                else if (size < 4) {
                    _is_valid = false;
                    return;
                }
                else {
                    cell.centre_frequency.push_back(uint64_t(GetUInt32(data)) * 10); // encoded unit is 10 Hz
                    data += 4; size -= 4;
                }

                if (size < 1) {
                    _is_valid = false;
                    return;
                }

                size_t len = data[0];
                data++; size--;
                while (len >= 5 && size >= 5) {
                    Subcell subcell;
                    subcell.cell_id_extension = data[0];
                    subcell.transposer_frequency = uint64_t(GetUInt32(data + 1)) * 10; // encoded unit is 10 Hz
                    data += 5; size -= 5; len -= 5;
                    cell.subcells.push_back(subcell);
                }

                cells.push_back(cell);
            }

            _is_valid = size == 0;
        }
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML and display.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration SisoNames({
        {u"SISO", 0},
        {u"MISO", 1},
    });
    const ts::Enumeration BandwidthNames({
        {u"8MHz",     0},
        {u"7MHz",     1},
        {u"6MHz",     2},
        {u"5MHz",     3},
        {u"10MHz",    4},
        {u"1.712MHz", 5},
    });
    const ts::Enumeration GuardIntervalNames({
        {u"1/32",   0},
        {u"1/16",   1},
        {u"1/8",    2},
        {u"1/4",    3},
        {u"1/128",  4},
        {u"19/128", 5},
        {u"19/256", 6},
    });
    const ts::Enumeration TransmissionModeNames({
        {u"2k",  0},
        {u"8k",  1},
        {u"4k",  2},
        {u"1k",  3},
        {u"16k", 4},
        {u"32k", 5},
    });
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::T2DeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        const uint8_t plp = data[0];
        const uint16_t sys = GetUInt16(data + 1);
        data += 3; size -= 3;
        strm << margin << UString::Format(u"PLP id: 0x%X (%d), T2 system id: 0x%X (%d)", {plp, plp, sys, sys}) << std::endl;

        if (size >= 2) {
            const bool tfs = (data[1] & 0x01) != 0;
            strm << margin << UString::Format(u"SISO/MISO: %s", {SisoNames.name((data[0] >> 6) & 0x03)}) << std::endl
                 << margin << UString::Format(u"Bandwidth: %s", {BandwidthNames.name((data[0] >> 2) & 0x0F)}) << std::endl
                 << margin << UString::Format(u"Guard interval: %s", {GuardIntervalNames.name((data[1] >> 5) & 0x07)}) << std::endl
                 << margin << UString::Format(u"Transmission mode: %s", {TransmissionModeNames.name((data[1] >> 2) & 0x07)}) << std::endl
                 << margin << UString::Format(u"Other frequency: %s", {(data[1] & 0x02) != 0}) << std::endl
                 << margin << UString::Format(u"TFS arrangement: %s", {tfs}) << std::endl;
            data += 2; size -= 2;

            while (size >= 3) {
                const uint16_t cell = GetUInt16(data);
                data += 2; size -= 2;
                strm << margin << UString::Format(u"- Cell id: 0x%X (%d)", {cell, cell}) << std::endl;

                if (tfs) {
                    size_t len = data[0];
                    data++; size--;
                    while (len >= 4 && size >= 4) {
                        strm << margin << UString::Format(u"  Centre frequency: %'d Hz", {uint64_t(GetUInt32(data)) * 10}) << std::endl;
                        data += 4; size -= 4; len -= 4;
                    }
                    if (len > 0) {
                        break;
                    }
                }
                else if (size < 4) {
                    break;
                }
                else {
                    strm << margin << UString::Format(u"  Centre frequency: %'d Hz", {uint64_t(GetUInt32(data)) * 10}) << std::endl;
                    data += 4; size -= 4;
                }

                if (size < 1) {
                    break;
                }

                size_t len = data[0];
                data++; size--;
                while (len >= 5 && size >= 5) {
                    strm << margin
                         << UString::Format(u"  Cell id ext: 0x%X (%d), transp. frequency: %'d Hz", {data[0], data[0], uint64_t(GetUInt32(data + 1)) * 10})
                         << std::endl;
                    data += 5; size -= 5; len -= 5;
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::T2DeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"plp_id", plp_id, true);
    root->setIntAttribute(u"T2_system_id", T2_system_id, true);
    if (has_extension) {
        xml::Element* ext = root->addElement(u"extension");
        ext->setIntEnumAttribute(SisoNames, u"SISO_MISO", SISO_MISO);
        ext->setIntEnumAttribute(BandwidthNames, u"bandwidth", bandwidth);
        ext->setIntEnumAttribute(GuardIntervalNames, u"guard_interval", guard_interval);
        ext->setIntEnumAttribute(TransmissionModeNames, u"transmission_mode", transmission_mode);
        ext->setBoolAttribute(u"other_frequency", other_frequency);
        ext->setBoolAttribute(u"tfs", tfs);
        for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
            xml::Element* ce = ext->addElement(u"cell");
            ce->setIntAttribute(u"cell_id", it1->cell_id, true);
            for (auto it2 = it1->centre_frequency.begin(); it2 != it1->centre_frequency.end(); ++it2) {
                ce->addElement(u"centre_frequency")->setIntAttribute(u"value", *it2, false);
            }
            for (auto it2 = it1->subcells.begin(); it2 != it1->subcells.end(); ++it2) {
                xml::Element* sub = ce->addElement(u"subcell");
                sub->setIntAttribute(u"cell_id_extension", it2->cell_id_extension, true);
                sub->setIntAttribute(u"transposer_frequency", it2->transposer_frequency, false);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::T2DeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector ext;
    bool ok =
        element->getIntAttribute(plp_id, u"plp_id", true) &&
        element->getIntAttribute(T2_system_id, u"T2_system_id", true) &&
        element->getChildren(ext, u"extension", 0, 1);

    has_extension = ok && !ext.empty();

    if (has_extension) {
        xml::ElementVector xcells;
        ok = ext[0]->getIntEnumAttribute(SISO_MISO, SisoNames, u"SISO_MISO", true) &&
             ext[0]->getIntEnumAttribute(bandwidth, BandwidthNames, u"bandwidth", true) &&
             ext[0]->getIntEnumAttribute(guard_interval, GuardIntervalNames, u"guard_interval", true) &&
             ext[0]->getIntEnumAttribute(transmission_mode, TransmissionModeNames, u"transmission_mode", true) &&
             ext[0]->getBoolAttribute(other_frequency, u"other_frequency", true) &&
             ext[0]->getBoolAttribute(tfs, u"tfs", true) &&
             ext[0]->getChildren(xcells, u"cell");

        for (size_t i1 = 0; ok && i1 < xcells.size(); ++i1) {
            xml::ElementVector xfreq;
            xml::ElementVector xsub;
            Cell cell;
            ok = xcells[i1]->getIntAttribute(cell.cell_id, u"cell_id", true) &&
                 xcells[i1]->getChildren(xfreq, u"centre_frequency", tfs ? 0 : 1, tfs ? xml::UNLIMITED : 1) &&
                 xcells[i1]->getChildren(xsub, u"subcell");

            for (size_t i2 = 0; ok && i2 < xfreq.size(); ++i2) {
                uint64_t freq = 0;
                ok = xfreq[i2]->getIntAttribute(freq, u"value", true);
                cell.centre_frequency.push_back(freq);
            }

            for (size_t i2 = 0; ok && i2 < xsub.size(); ++i2) {
                Subcell sub;
                ok = xsub[i2]->getIntAttribute(sub.cell_id_extension, u"cell_id_extension", true) &&
                     xsub[i2]->getIntAttribute(sub.transposer_frequency, u"transposer_frequency", true);
                cell.subcells.push_back(sub);
            }

            cells.push_back(cell);
        }
    }
    return ok;
}
