//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTOT.h"
#include "tsTDT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"TOT"
#define MY_CLASS ts::TOT
#define MY_TID ts::TID_TOT
#define MY_PID ts::PID_TOT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TOT::TOT(const Time& utc_time_) :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    utc_time(utc_time_),
    regions(),
    descs(this),
    _time_reference_offset(0)
{
}

ts::TOT::TOT(DuckContext& duck, const BinaryTable& table) :
    TOT()
{
    deserialize(duck, table);
}

ts::TOT::TOT(const TOT& other) :
    AbstractTable(other),
    utc_time(other.utc_time),
    regions(other.regions),
    descs(this, other.descs),
    _time_reference_offset(other._time_reference_offset)
{
}


//----------------------------------------------------------------------------
// Check if the sections of this table have a trailing CRC32.
//----------------------------------------------------------------------------

bool ts::TOT::useTrailingCRC32() const
{
    // A TOT is a short section with a CRC32.
    return true;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::TOT::clearContent()
{
    utc_time.clear();
    regions.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Return the local time according to a region description
//----------------------------------------------------------------------------

ts::Time ts::TOT::localTime(const Region& reg) const
{
    // Add local time offset in milliseconds.
    // In case of non-standard time reference, the offset in the descriptor
    // is an offset from the non-standard time reference, not from UTC.
    return utc_time + _time_reference_offset + MilliSecond(reg.time_offset) * 60 * MilliSecPerSec;
}


//----------------------------------------------------------------------------
// Format a time offset in minutes
//----------------------------------------------------------------------------

ts::UString ts::TOT::timeOffsetFormat(int minutes)
{
    return UString::Format(u"%s%02d:%02d", {minutes < 0 ? u"-" : u"", std::abs(minutes) / 60, std::abs(minutes) % 60});
}


//----------------------------------------------------------------------------
// Add descriptors, filling regions from local_time_offset_descriptor's.
//----------------------------------------------------------------------------

void ts::TOT::addDescriptors(DuckContext& duck, const DescriptorList& dlist)
{
    // Loop on all descriptors.
    for (size_t index = 0; index < dlist.count(); ++index) {
        if (!dlist[index].isNull() && dlist[index]->isValid()) {
            if (dlist[index]->tag() != DID_LOCAL_TIME_OFFSET) {
                // Not a local_time_offset_descriptor, add to descriptor list.
                descs.add(dlist[index]);
            }
            else {
                // Decode local_time_offset_descriptor in the list of regions.
                LocalTimeOffsetDescriptor lto(duck, *dlist[index]);
                if (lto.isValid()) {
                    regions.insert(regions.end(), lto.regions.begin(), lto.regions.end());
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TOT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // A TOT section is a short section with a CRC32. But it has already been checked
    // and removed from the buffer since TOT::useTrailingCRC32() returns true.

    // Get UTC time. The time reference is UTC as defined by DVB, but can be non-standard.
    _time_reference_offset = buf.duck().timeReferenceOffset();
    utc_time = buf.getFullMJD() - _time_reference_offset;

    // Get descriptor list.
    DescriptorList dlist(nullptr);
    buf.getDescriptorListWithLength(dlist);

    // Split between actual descriptors and regions.
    addDescriptors(buf.duck(), dlist);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TOT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Encode the data in MJD in the payload. Defined as UTC by DVB, but can be non-standard.
    _time_reference_offset = buf.duck().timeReferenceOffset();
    buf.putFullMJD(utc_time + _time_reference_offset);

    // Build a descriptor list.
    DescriptorList dlist(nullptr);

    // Add all regions in one or more local_time_offset_descriptor.
    LocalTimeOffsetDescriptor lto;
    for (auto& reg : regions) {
        lto.regions.push_back(reg);
        if (lto.regions.size() >= LocalTimeOffsetDescriptor::MAX_REGION) {
            dlist.add(buf.duck(), lto);
            lto.regions.clear();
        }
    }
    if (!lto.regions.empty()) {
        dlist.add(buf.duck(), lto);
    }

    // Append the "other" descriptors to the list
    dlist.add(descs);

    // Insert descriptor list (with leading length field).
    buf.putPartialDescriptorListWithLength(dlist);

    // A TOT section is a short section with a CRC32. But it will be
    // automatically added since TOT::useTrailingCRC32() returns true.
}


//----------------------------------------------------------------------------
// A static method to display a TOT section.
//----------------------------------------------------------------------------

void ts::TOT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(5)) {
        // Use TDT display routine for the beginning of the section (adjusted UTC time).
        TDT::DisplaySection(disp, section, buf, margin);
        disp.displayDescriptorListWithLength(section, buf, margin);
        disp.displayCRC32(section, buf, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TOT::buildXML(DuckContext& duck, xml::Element* root) const
{
    // Always cache this value.
    _time_reference_offset = duck.timeReferenceOffset();

    root->setDateTimeAttribute(u"UTC_time", utc_time);

    // Add one local_time_offset_descriptor per set of regions.
    // Each local_time_offset_descriptor can contain up to 19 regions.
    LocalTimeOffsetDescriptor lto;
    for (auto& reg : regions) {
        lto.regions.push_back(reg);
        if (lto.regions.size() >= LocalTimeOffsetDescriptor::MAX_REGION) {
            // The descriptor is full, flush it in the list.
            lto.toXML(duck, root);
            lto.regions.clear();
        }
    }
    if (!lto.regions.empty()) {
        // The descriptor is not empty, flush it in the list.
        lto.toXML(duck, root);
    }

    // Add other descriptors.
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TOT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    // Always cache this value.
    _time_reference_offset = duck.timeReferenceOffset();

    DescriptorList orig(this);

    // Get all descriptors in a separated list.
    const bool ok = element->getDateTimeAttribute(utc_time, u"UTC_time", true) && orig.fromXML(duck, element);

    // Then, split local_time_offset_descriptor and others.
    addDescriptors(duck, orig);
    return ok;
}
