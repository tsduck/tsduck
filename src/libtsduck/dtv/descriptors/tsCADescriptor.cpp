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

#include "tsCADescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsDescriptorList.h"
TSDUCK_SOURCE;

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
    ca_pid(ca_pid_),
    private_data()
{
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

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
// Serialization
//----------------------------------------------------------------------------

void ts::CADescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(cas_id);
    bbp->appendUInt16(0xE000 | (ca_pid & 0x1FFF));
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CADescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 4;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        cas_id = GetUInt16 (data);
        ca_pid = GetUInt16 (data + 2) & 0x1FFF;
        private_data.copy (data + 4, size - 4);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CADescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size < 4) {
        display.displayExtraData(data, size, indent);
    }
    else {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        // Extract common part
        uint16_t sysid = GetUInt16(data);
        uint16_t pid = GetUInt16(data + 2) & 0x1FFF;
        const UChar* const dtype = tid == TID_CAT ? u"EMM" : (tid == TID_PMT ? u"ECM" : u"CA");
        data += 4; size -= 4;

        strm << margin << UString::Format(u"CA System Id: %s, %s PID: %d (0x%X)", {names::CASId(duck, sysid, names::FIRST), dtype, pid, pid}) << std::endl;

        // CA private part.
        if (size > 0) {
            // Check if a specific CAS registered its own display routine.
            DisplayCADescriptorFunction disp = PSIRepository::Instance()->getCADescriptorDisplay(sysid);
            if (disp != nullptr) {
                // Use a CAS-specific display routine.
                disp(display, data, size, indent, tid);
            }
            else {
                display.displayPrivateData(u"Private CA data", data, size, indent);
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
    return element->getIntAttribute<uint16_t>(cas_id, u"CA_system_id", true, 0, 0x0000, 0xFFFF) &&
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
