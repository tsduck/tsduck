//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAMT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AMT"
#define MY_CLASS ts::AMT
#define MY_TID ts::TID_AMT
#define MY_PID ts::PID_AMT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::AMT::AMT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur)
{
}

ts::AMT::AMT(DuckContext& duck, const BinaryTable& table) :
    AMT()
{
    deserialize(duck, table);
}

void ts::AMT::clearContent()
{
    services.clear();
}

uint16_t ts::AMT::tableIdExtension() const
{
    return 0; // always zero
}


//----------------------------------------------------------------------------
// Evaluate the binary size of the service entry.
//----------------------------------------------------------------------------

size_t ts::AMT::Service::binarySize() const
{
    return 4 + src.binarySize() + 1 + dst.binarySize() + 1 + private_data.size();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AMT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    [[maybe_unused]] const uint16_t num_of_service_id = buf.getBits<uint16_t>(10);
    buf.skipReservedBits(6);

    while (buf.canRead()) {
        auto& srv(services[buf.getUInt16()]);
        const uint8_t ip_version = buf.getBit();
        buf.skipReservedBits(5);
        buf.pushReadSizeFromLength(10);
        if (ip_version == 0) {
            // Two IPv4 address/prefix.
            srv.src.setAddress4(buf.getUInt32());
            srv.src.setPrefixSize(buf.getUInt8());
            srv.dst.setAddress4(buf.getUInt32());
            srv.dst.setPrefixSize(buf.getUInt8());
        }
        else {
            // Two IPv6 address/prefix.
            srv.src.setAddress(buf.getBytes(IPAddress::BYTES6));
            srv.src.setPrefixSize(buf.getUInt8());
            srv.dst.setAddress(buf.getBytes(IPAddress::BYTES6));
            srv.dst.setPrefixSize(buf.getUInt8());
        }
        buf.getBytesAppend(srv.private_data);
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AMT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Need to write the number of services in each section.
    size_t num_of_service_id = 0;
    buf.putBits(num_of_service_id, 10);
    buf.putReserved(6);

    for (const auto& it : services) {
        // Check that the two IP addresses have the same type.
        if (it.second.src.generation() != it.second.dst.generation()) {
            buf.setUserError();
            return;
        }

        // Check if we can write this entry in the current section.
        const size_t srv_size = it.second.binarySize();
        if (srv_size >= buf.remainingWriteBytes()) {
            if (num_of_service_id == 0) {
                // Cannot fit and this is the first in the section => too large anyway
                buf.setUserError();
                return;
            }
            // Close current section and starts a new one.
            buf.pushState();
            buf.writeSeek(0);
            buf.putBits(num_of_service_id, 10);
            buf.popState();
            addOneSection(table, buf);
        }

        // Serialize the service entry.
        buf.putUInt16(it.first);
        buf.putBit(it.second.src.generation() == IP::v6);
        buf.putReserved(5);
        buf.pushWriteSequenceWithLeadingLength(10);
        if (it.second.src.generation() == IP::v4) {
            buf.putUInt32(it.second.src.address4());
            buf.putUInt8(uint8_t(it.second.src.prefixSize()));
            buf.putUInt32(it.second.dst.address4());
            buf.putUInt8(uint8_t(it.second.dst.prefixSize()));
        }
        else {
            buf.putBytes(it.second.src.address6());
            buf.putUInt8(uint8_t(it.second.src.prefixSize()));
            buf.putBytes(it.second.dst.address6());
            buf.putUInt8(uint8_t(it.second.dst.prefixSize()));
        }
        buf.putBytes(it.second.private_data);
        buf.popState();
        num_of_service_id++;
    }

    // Close last section.
    buf.pushState();
    buf.writeSeek(0);
    buf.putBits(num_of_service_id, 10);
    buf.popState();
}


//----------------------------------------------------------------------------
// A static method to display a AMT section.
//----------------------------------------------------------------------------

void ts::AMT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Number of services: " << buf.getBits<uint16_t>(10) << std::endl;
    buf.skipReservedBits(6);

    while (buf.canReadBytes(14)) {
        disp << margin << UString::Format(u"- Service id: %n", buf.getUInt16()) << std::endl;
        const uint8_t ip_version = buf.getBit();
        buf.skipReservedBits(5);
        buf.pushReadSizeFromLength(10);
        if (ip_version == 0) {
            // Two IPv4 address/prefix.
            IPAddressMask am;
            am.setAddress4(buf.getUInt32());
            am.setPrefixSize(buf.getUInt8());
            disp << margin << "  Source: " << am << std::endl;
            am.setAddress4(buf.getUInt32());
            am.setPrefixSize(buf.getUInt8());
            disp << margin << "  Destination: " << am << std::endl;
        }
        else {
            // Two IPv6 address/prefix.
            IPAddressMask am;
            am.setAddress(buf.getBytes(IPAddress::BYTES6));
            am.setPrefixSize(buf.getUInt8());
            disp << margin << "  Source: " << am << std::endl;
            am.setAddress(buf.getBytes(IPAddress::BYTES6));
            am.setPrefixSize(buf.getUInt8());
            disp << margin << "  Destination: " << am << std::endl;
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin + u"  ");
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AMT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    for (const auto& it : services) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.first, true);
        e->setIPAttribute(u"src", it.second.src);
        e->setIPAttribute(u"dst", it.second.dst);
        e->addHexaTextChild(u"private_data", it.second.private_data, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AMT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xsrv;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", false, true) &&
        element->getChildren(xsrv, u"service");

    for (const auto& it : xsrv) {
        uint16_t service_id = 0;
        ok = it->getIntAttribute(service_id, u"service_id", true) && ok;
        if (!ok) {
            break;
        }
        else if (services.contains(service_id)) {
            element->report().error(u"duplicate service_id %n in <%s>, line %d", service_id, element->name(), element->lineNumber());
        }
        else {
            auto& srv(services[service_id]);
            ok = it->getIPAttribute(srv.src, u"src", true) &&
                 it->getIPAttribute(srv.dst, u"dst", true) &&
                 it->getHexaTextChild(srv.private_data, u"private_data") &&
                 ok;
        }
    }
    return ok;
}
