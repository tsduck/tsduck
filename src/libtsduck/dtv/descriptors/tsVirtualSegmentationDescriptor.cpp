//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVirtualSegmentationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsIntegerUtils.h"

#define MY_XML_NAME u"virtual_segmentation_descriptor"
#define MY_CLASS ts::VirtualSegmentationDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_VIRT_SEGMENT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VirtualSegmentationDescriptor::VirtualSegmentationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::VirtualSegmentationDescriptor::VirtualSegmentationDescriptor(DuckContext& duck, const Descriptor& desc) :
    VirtualSegmentationDescriptor()
{
    deserialize(duck, desc);
}

void ts::VirtualSegmentationDescriptor::clearContent()
{
    ticks_per_second.reset();
    partitions.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::VirtualSegmentationDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::VirtualSegmentationDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (partitions.size() > MAX_PARTITION) {
        buf.setUserError();
    }
    else if (ticks_per_second.has_value() || !partitions.empty()) {
        // Compute the maximum size in bits of all maximum_duration fields.
        // This is required to compute maximum_duration_length_minus_1 (MDL).
        size_t mdl_bits = 0;
        for (const auto& it : partitions) {
            mdl_bits = std::max(mdl_bits, BitSize(it.maximum_duration.value_or(0)));
        }

        // MDL (max duration length) is the number of additional bytes, beyond the first 5 bits, in max_duration.
        // MDL is stored on 2 bits and must be in the range 0..3.
        // The maximum size of the max_duration field is consequently 29 bits.
        const size_t mdl = mdl_bits <= 5 ? 0 : (std::min<size_t>(29, mdl_bits) - 5 + 7) / 8;

        // Fixed part.
        const bool timescale_flag = ticks_per_second.has_value() || mdl > 0;
        buf.putBits(partitions.size(), 3);
        buf.putBit(timescale_flag);
        buf.putBits(0xFF, 4);

        if (timescale_flag) {
            buf.putBits(ticks_per_second.value_or(0), 21);
            buf.putBits(mdl, 2);
            buf.putBit(1);
        }

        for (const auto& it : partitions) {
            buf.putBit(!it.boundary_PID.has_value());
            buf.putBits(it.partition_id, 3);
            buf.putBits(0xFF, 4);
            buf.putBits(it.SAP_type_max, 3);
            if (it.boundary_PID.has_value()) {
                buf.putBits(0xFF, 5);
                buf.putBits(it.boundary_PID.value(), 13);
                buf.putBits(0xFF, 3);
            }
            else {
                buf.putBits(it.maximum_duration.value_or(0), mdl * 8 + 5);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VirtualSegmentationDescriptor::deserializePayload(PSIBuffer& buf)
{
    if (buf.canRead()) {
        size_t mdl = 0;
        const size_t num_partitions = buf.getBits<size_t>(3);
        const bool timescale_flag = buf.getBool();
        buf.skipBits(4);
        if (timescale_flag) {
            buf.getBits(ticks_per_second, 21);
            buf.getBits(mdl, 2);
            buf.skipBits(1);
        }
        for (size_t i = 0; i < num_partitions && buf.canRead(); ++i) {
            Partition part;
            const bool explicit_boundary_flag = buf.getBool();
            buf.getBits(part.partition_id, 3);
            buf.skipBits(4);
            buf.getBits(part.SAP_type_max, 3);
            if (!explicit_boundary_flag) {
                buf.skipBits(5);
                buf.getBits(part.boundary_PID, 13);
                buf.skipBits(3);
            }
            else {
                part.maximum_duration = buf.getBits<uint32_t>(mdl * 8 + 5);
            }
            partitions.push_back(part);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VirtualSegmentationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        size_t mdl = 0;
        const size_t num_partitions = buf.getBits<size_t>(3);
        const bool timescale_flag = buf.getBool();
        buf.skipBits(4);

        if (timescale_flag && buf.canReadBytes(3)) {
            disp << margin << UString::Format(u"Ticks per seconds: %'d", {buf.getBits<uint32_t>(21)}) << std::endl;
            buf.getBits(mdl, 2);
            disp << margin << UString::Format(u"Maximum duration length: %d bytes + 5 bits", {mdl}) << std::endl;
            buf.skipBits(1);
        }

        for (size_t i = 0; i < num_partitions && buf.canReadBytes(2); ++i) {
            const bool explicit_boundary_flag = buf.getBool();
            disp << margin << UString::Format(u"- Partition id: %d", {buf.getBits<uint8_t>(3)});
            buf.skipBits(4);
            disp << UString::Format(u", SAP type max: %d", {buf.getBits<uint8_t>(3)}) << std::endl;
            if (!explicit_boundary_flag) {
                buf.skipBits(5);
                disp << margin << UString::Format(u"  Boundary PID: 0x%X (%<d)", {buf.getBits<uint16_t>(13)}) << std::endl;
                buf.skipBits(3);
            }
            else if (buf.remainingReadBits() < mdl * 8 + 5) {
                buf.setUserError();
            }
            else {
                disp << margin << UString::Format(u"  Maximum duration: %'d ticks", {buf.getBits<uint32_t>(mdl * 8 + 5)}) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::VirtualSegmentationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setOptionalIntAttribute(u"ticks_per_second", ticks_per_second);
    for (const auto& it : partitions) {
        xml::Element* e = root->addElement(u"partition");
        e->setIntAttribute(u"partition_id", it.partition_id);
        e->setIntAttribute(u"SAP_type_max", it.SAP_type_max);
        e->setOptionalIntAttribute(u"boundary_PID", it.boundary_PID, true);
        e->setOptionalIntAttribute(u"maximum_duration", it.maximum_duration);
    }
}

bool ts::VirtualSegmentationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xpart;
    bool ok =
        element->getOptionalIntAttribute(ticks_per_second, u"ticks_per_second", 0, 0x001FFFFF) &&
        element->getChildren(xpart, u"partition", 0, MAX_PARTITION);

    for (auto it = xpart.begin(); ok && it != xpart.end(); ++it) {
        Partition part;
        ok = (*it)->getIntAttribute(part.partition_id, u"partition_id", true, 0, 0, 7) &&
             (*it)->getIntAttribute(part.SAP_type_max, u"SAP_type_max", true, 0, 0, 7) &&
             (*it)->getOptionalIntAttribute<PID>(part.boundary_PID, u"boundary_PID", 0, 0x1FFF) &&
             (*it)->getOptionalIntAttribute(part.maximum_duration, u"maximum_duration", 0, 0x1FFFFFFF);
        if (part.boundary_PID.has_value() && part.maximum_duration.has_value()) {
            element->report().error(u"attributes 'boundary_PID' and 'maximum_duration' are mutually exclusive in <%s>, line %d", {element->name(), (*it)->lineNumber()});
        }
        partitions.push_back(part);
    }
    return ok;
}
