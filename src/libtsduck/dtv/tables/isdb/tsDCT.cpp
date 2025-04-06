//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDCT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DCT"
#define MY_CLASS ts::DCT
#define MY_TID ts::TID_DCT
#define MY_PID ts::PID_DCT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DCT::DCT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur)
{
}

ts::DCT::DCT(DuckContext& duck, const BinaryTable& table) :
    DCT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::DCT::tableIdExtension() const
{
    return network_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DCT::clearContent()
{
    network_id = 0;
    transmission_rate = 0;
    streams.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    network_id = section.tableIdExtension();
    transmission_rate = buf.getUInt8();
    while (buf.canRead()) {
        streams.push_back(StreamInfo());
        StreamInfo& str(streams.back());
        str.transport_stream_id = buf.getUInt16();
        str.DL_PID = buf.getPID();
        str.ECM_PID = buf.getPID();
        buf.skipReservedBits(4);
        buf.pushReadSizeFromLength(12);
        while (buf.canRead()) {
            str.models.push_back(ModelInfo());
            ModelInfo& mod(str.models.back());
            mod.maker_id = buf.getUInt8();
            mod.model_id = buf.getUInt8();
            mod.version_id = buf.getUInt8();
            mod.DLT_size = buf.getUInt8();
        }
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt8(transmission_rate);
    buf.pushState();
    const size_t start = buf.currentWriteByteOffset();

    // Loop on new sections until all data bytes are gone.
    for (const auto& str : streams) {

        // Compute size of this transport stream entry.
        const size_t str_size = 8 + 4 * str.models.size();
        if (str_size > buf.remainingWriteBytes()) {
            // This transport stream entry won't fit in current section. Close it and open another one.
            addOneSection(table, buf);
            if (buf.currentWriteByteOffset() == start) {
                // This is the first entry in this section and it does not fit => too large.
                buf.setUserError();
                return;
            }
        }

        // Insert this transport stream entry in current section.
        buf.putUInt16(str.transport_stream_id);
        buf.putPID(str.DL_PID);
        buf.putPID(str.ECM_PID);
        buf.putReserved(4);
        buf.pushWriteSequenceWithLeadingLength(12);
        for (const auto& mod : str.models) {
            buf.putUInt8(mod.maker_id);
            buf.putUInt8(mod.model_id);
            buf.putUInt8(mod.version_id);
            buf.putUInt8(mod.DLT_size);
        }
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// A static method to display a DCT section.
//----------------------------------------------------------------------------

void ts::DCT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"DLT network id: %n", section.tableIdExtension()) << std::endl;
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Transmission rate: %d pkt/s", buf.getUInt8()) << std::endl;
    }
    while (buf.canReadBytes(8)) {
        disp << margin << UString::Format(u"- Transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"  Download PID: %n", buf.getPID());
        disp << UString::Format(u", ECM PID: %n", buf.getPID()) << std::endl;
        buf.skipReservedBits(4);
        buf.pushReadSizeFromLength(12);
        while (buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"  - Maker id: %n", buf.getUInt8());
            disp << UString::Format(u", model: %n", buf.getUInt8());
            disp << UString::Format(u", version: %n", buf.getUInt8()) << std::endl;
            disp << margin << UString::Format(u"    DLT size: %d sections", buf.getUInt8()) << std::endl;
        }
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"network_id", network_id, true);
    root->setIntAttribute(u"transmission_rate", transmission_rate);
    for (const auto& str : streams) {
        xml::Element* xstr = root->addElement(u"transport_stream");
        xstr->setIntAttribute(u"id", str.transport_stream_id, true);
        xstr->setIntAttribute(u"DL_PID", str.DL_PID, true);
        if (str.ECM_PID != PID_NULL) {
            xstr->setIntAttribute(u"ECM_PID", str.ECM_PID, true);
        }
        for (const auto& mod : str.models) {
            xml::Element* xmod = xstr->addElement(u"model");
            xmod->setIntAttribute(u"maker_id", mod.maker_id, true);
            xmod->setIntAttribute(u"model_id", mod.model_id, true);
            xmod->setIntAttribute(u"version_id", mod.version_id, true);
            xmod->setIntAttribute(u"DLT_size", mod.DLT_size, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xstr;
    bool ok = element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
              element->getBoolAttribute(_is_current, u"current", false, true) &&
              element->getIntAttribute(network_id, u"network_id", true) &&
              element->getIntAttribute(transmission_rate, u"transmission_rate", true) &&
              element->getChildren(xstr, u"transport_stream");
    for (size_t i = 0; ok && i < xstr.size(); ++i) {
        streams.push_back(StreamInfo());
        StreamInfo& str(streams.back());
        xml::ElementVector xmod;
        ok = xstr[i]->getIntAttribute(str.transport_stream_id, u"id", true) &&
             xstr[i]->getIntAttribute(str.DL_PID, u"DL_PID", true, PID_NULL, 0x0000, 0x1FFF) &&
             xstr[i]->getIntAttribute(str.ECM_PID, u"ECM_PID", false, PID_NULL, 0x0000, 0x1FFF) &&
             xstr[i]->getChildren(xmod, u"model");
        for (size_t j = 0; ok && j < xmod.size(); ++j) {
            str.models.push_back(ModelInfo());
            ModelInfo& mod(str.models.back());
            ok = xmod[j]->getIntAttribute(mod.maker_id, u"maker_id", true) &&
                 xmod[j]->getIntAttribute(mod.model_id, u"model_id", true) &&
                 xmod[j]->getIntAttribute(mod.version_id, u"version_id", true) &&
                 xmod[j]->getIntAttribute(mod.DLT_size, u"DLT_size", true);
        }
    }
    return ok;
}
