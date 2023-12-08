//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCADescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
#include "tsDescriptorList.h"

#define MY_XML_NAME u"CA_descriptor"
#define MY_CLASS ts::CADescriptor
#define MY_DID ts::DID_CA
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CADescriptor::CADescriptor(uint16_t cas_id_, PID ca_pid_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    cas_id(cas_id_),
    ca_pid(ca_pid_)
{
}

void ts::CADescriptor::clearContent()
{
    cas_id = 0;
    ca_pid = PID_NULL;
    private_data.clear();
}

ts::CADescriptor::CADescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    cas_id(0),
    ca_pid(PID_NULL),
    private_data()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Exactly identical CA_descriptors shall not be dumplicated.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::CADescriptor::duplicationMode() const
{
    return DescriptorDuplication::ADD_OTHER;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CADescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(cas_id);
    buf.putPID(ca_pid);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CADescriptor::deserializePayload(PSIBuffer& buf)
{
    cas_id = buf.getUInt16();
    ca_pid = buf.getPID();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CADescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        // Display common part
        const uint16_t casid = buf.getUInt16();
        disp << margin << "CA System Id: " << names::CASId(disp.duck(), casid, NamesFlags::FIRST);
        disp << ", " << (tid == TID_CAT ? u"EMM" : (tid == TID_PMT ? u"ECM" : u"CA"));
        disp << UString::Format(u" PID: %d (0x%<X)", {buf.getPID()}) << std::endl;

        // CA private part.
        if (buf.canRead()) {
            // Check if a specific CAS registered its own display routine.
            DisplayCADescriptorFunction func = PSIRepository::Instance().getCADescriptorDisplay(casid);
            if (func != nullptr) {
                // Use a CAS-specific display routine.
                func(disp, buf, margin, tid);
            }
            else {
                disp.displayPrivateData(u"Private CA data", buf, NPOS, margin);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CADescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", cas_id, true);
    root->setIntAttribute(u"CA_PID", ca_pid, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CADescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(cas_id, u"CA_system_id", true, 0, 0x0000, 0xFFFF) &&
           element->getIntAttribute<PID>(ca_pid, u"CA_PID", true, 0, 0x0000, 0x1FFF) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}


//----------------------------------------------------------------------------
// Decode a command-line CA_descriptor and fills this object with it.
//----------------------------------------------------------------------------

bool ts::CADescriptor::fromCommmandLine(const UString& value, Report& report)
{
    private_data.clear();

    int casid = 0;
    int pid = 0;
    size_t count = 0;
    size_t index = 0;

    value.scan(count, index, u"%i/%i", {&casid, &pid});

    // On return, index points to the next index in val after "cas-id/PID".
    // If there is a private part, then index must points to a '/'.
    if (count != 2 || casid < 0 || casid > 0xFFFF || pid < 0 || pid >= int(PID_MAX) || (index < value.length() && value[index] != u'/')) {
        report.error(u"invalid \"cas-id/PID[/private-data]\" value \"%s\"", {value});
        return false;
    }

    cas_id = uint16_t(casid);
    ca_pid = PID(pid);

    if (index < value.length()) {
        // There is a private part
        const UString hexa(value.substr(index + 1));
        if (!hexa.hexaDecode(private_data)) {
            report.error(u"invalid private data \"%s\" for CA_descriptor, specify an even number of hexa digits", {hexa});
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Decode command-line CA_descriptor and add them in a descriptor list.
//----------------------------------------------------------------------------

bool ts::CADescriptor::AddFromCommandLine(DuckContext& duck, DescriptorList& dlist, const UStringVector& values)
{
    bool result = true;
    for (size_t i = 0; i < values.size(); ++i) {
        CADescriptor desc;
        if (desc.fromCommmandLine(values[i], duck.report())) {
            dlist.add(duck, desc);
        }
        else {
            result = false;
        }
    }
    return result;
}


//----------------------------------------------------------------------------
// Static method to search a CA_descriptor by ECM/EMM PID.
//----------------------------------------------------------------------------

size_t ts::CADescriptor::SearchByPID(const ts::DescriptorList& dlist, ts::PID pid, size_t start_index)
{
    bool found = false;
    for (; !found && start_index < dlist.count(); start_index++) {
        const DescriptorPtr& desc(dlist[start_index]);
        found = !desc.isNull() &&
            desc->isValid() &&
            desc->tag() == DID_CA &&
            desc->payloadSize() >= 4 &&
            (GetUInt16(desc->payload() + 2) & 0x1FFF) == pid;
    }
    return start_index;
}


//----------------------------------------------------------------------------
// Static method to search a CA_descriptor by CA system id.
//----------------------------------------------------------------------------

size_t ts::CADescriptor::SearchByCAS(const ts::DescriptorList& dlist, uint16_t casid, size_t start_index)
{
    bool found = false;
    for (; !found && start_index < dlist.count(); start_index++) {
        const DescriptorPtr& desc(dlist[start_index]);
        found = !desc.isNull() &&
            desc->isValid() &&
            desc->tag() == DID_CA &&
            desc->payloadSize() >= 4 &&
            GetUInt16(desc->payload()) == casid;
    }
    return start_index;
}
