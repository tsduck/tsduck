//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsT2DeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"T2_delivery_system_descriptor"
#define MY_CLASS ts::T2DeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_T2_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::T2DeliverySystemDescriptor::T2DeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_T2, MY_XML_NAME)
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


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::T2DeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::T2DeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(plp_id);
    buf.putUInt16(T2_system_id);
    if (has_extension) {
        buf.putBits(SISO_MISO, 2);
        buf.putBits(bandwidth, 4);
        buf.putBits(0xFF, 2);
        buf.putBits(guard_interval, 3);
        buf.putBits(transmission_mode, 3);
        buf.putBit(other_frequency);
        buf.putBit(tfs);
        for (const auto& it1 : cells) {
            buf.putUInt16(it1.cell_id);
            if (tfs) {
                buf.pushWriteSequenceWithLeadingLength(8); // frequency_loop_length
                for (const auto& it2 : it1.centre_frequency) {
                    buf.putUInt32(uint32_t(it2 / 10)); // encoded in units of 10 Hz
                }
                buf.popState(); // update frequency_loop_length
            }
            else {
                buf.putUInt32(uint32_t((it1.centre_frequency.empty() ? 0 : it1.centre_frequency.front()) / 10)); // encoded in units of 10 Hz
            }
            buf.pushWriteSequenceWithLeadingLength(8); // subcell_info_loop_length
            for (const auto& it2 : it1.subcells) {
                buf.putUInt8(it2.cell_id_extension);
                buf.putUInt32(uint32_t(it2.transposer_frequency / 10)); // encoded in units of 10 Hz
            }
            buf.popState(); // update subcell_info_loop_length
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::T2DeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    plp_id = buf.getUInt8();
    T2_system_id = buf.getUInt16();
    has_extension = buf.canRead();

    if (has_extension) {
        buf.getBits(SISO_MISO, 2);
        buf.getBits(bandwidth, 4);
        buf.skipBits(2);
        buf.getBits(guard_interval, 3);
        buf.getBits(transmission_mode, 3);
        other_frequency = buf.getBool();
        tfs = buf.getBool();
        while (buf.canRead()) {
            Cell cell;
            cell.cell_id = buf.getUInt16();
            if (tfs) {
                buf.pushReadSizeFromLength(8); // frequency_loop_length
                while (buf.canRead()) {
                    cell.centre_frequency.push_back(uint64_t(buf.getUInt32()) * 10); // encoded unit is 10 Hz
                }
                buf.popState(); // frequency_loop_length
            }
            else {
                cell.centre_frequency.push_back(uint64_t(buf.getUInt32()) * 10); // encoded unit is 10 Hz
            }
            buf.pushReadSizeFromLength(8); // subcell_info_loop_length
            while (buf.canRead()) {
                Subcell subcell;
                subcell.cell_id_extension = buf.getUInt8();
                subcell.transposer_frequency = uint64_t(buf.getUInt32()) * 10; // encoded unit is 10 Hz
                cell.subcells.push_back(subcell);
            }
            buf.popState(); // subcell_info_loop_length
            cells.push_back(cell);
        }
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML and disp.
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

void ts::T2DeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"PLP id: 0x%X (%<d)", {buf.getUInt8()});
        disp << UString::Format(u", T2 system id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;

        if (buf.canReadBytes(2)) {
            disp << margin << "SISO/MISO: " << SisoNames.name(buf.getBits<uint8_t>(2)) << std::endl;
            disp << margin << "Bandwidth: " << BandwidthNames.name(buf.getBits<uint8_t>(4)) << std::endl;
            buf.skipBits(2);
            disp << margin << "Guard interval: " << GuardIntervalNames.name(buf.getBits<uint8_t>(3)) << std::endl;
            disp << margin << "Transmission mode: " << TransmissionModeNames.name(buf.getBits<uint8_t>(3)) << std::endl;
            disp << margin << UString::Format(u"Other frequency: %s", {buf.getBool()}) << std::endl;
            const bool tfs = buf.getBool();
            disp << margin << UString::Format(u"TFS arrangement: %s", {tfs}) << std::endl;

            while (buf.canReadBytes(3)) {
                disp << margin << UString::Format(u"- Cell id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                if (tfs) {
                    buf.pushReadSizeFromLength(8); // frequency_loop_length
                    while (buf.canRead()) {
                        disp << margin << UString::Format(u"  Centre frequency: %'d Hz", {uint64_t(buf.getUInt32()) * 10}) << std::endl;
                    }
                    buf.popState(); // frequency_loop_length
                }
                else if (buf.canReadBytes(4)) {
                    disp << margin << UString::Format(u"  Centre frequency: %'d Hz", {uint64_t(buf.getUInt32()) * 10}) << std::endl;
                }
                buf.pushReadSizeFromLength(8); // subcell_info_loop_length
                while (buf.canReadBytes(5)) {
                    disp << margin << UString::Format(u"  Cell id ext: 0x%X (%<d)", {buf.getUInt8()});
                    disp << UString::Format(u", transp. frequency: %'d Hz", {uint64_t(buf.getUInt32()) * 10}) << std::endl;
                }
                buf.popState(); // subcell_info_loop_length
            }
        }
    }
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
        for (const auto& it1 : cells) {
            xml::Element* ce = ext->addElement(u"cell");
            ce->setIntAttribute(u"cell_id", it1.cell_id, true);
            for (const auto& it2 : it1.centre_frequency) {
                ce->addElement(u"centre_frequency")->setIntAttribute(u"value", it2, false);
            }
            for (const auto& it2 : it1.subcells) {
                xml::Element* sub = ce->addElement(u"subcell");
                sub->setIntAttribute(u"cell_id_extension", it2.cell_id_extension, true);
                sub->setIntAttribute(u"transposer_frequency", it2.transposer_frequency, false);
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
