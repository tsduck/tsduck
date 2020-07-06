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

#include "tsCableDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsBCD.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"cable_delivery_system_descriptor"
#define MY_CLASS ts::CableDeliverySystemDescriptor
#define MY_DID ts::DID_CABLE_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CableDeliverySystemDescriptor::CableDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_C, MY_XML_NAME),
    frequency(0),
    FEC_outer(0),
    modulation(0),
    symbol_rate(0),
    FEC_inner(0)
{
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

void ts::CableDeliverySystemDescriptor::clearContent()
{
    frequency = 0;
    FEC_outer = 0;
    modulation = 0;
    symbol_rate = 0;
    FEC_inner = 0;
}

ts::CableDeliverySystemDescriptor::CableDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    CableDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CableDeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendBCD(uint32_t(frequency / 100), 8);  // coded in 100 Hz units
    bbp->appendUInt16(0xFFF0 | FEC_outer);
    bbp->appendUInt8(modulation);
    bbp->appendBCD(uint32_t(symbol_rate / 100), 7, true, FEC_inner);  // coded in 100 sym/s units, FEC in last nibble
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CableDeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() == 11)) {
        return;
    }

    const uint8_t* data = desc.payload();

    frequency = 100 * uint64_t(DecodeBCD(data, 8));  // coded in 100 Hz units
    FEC_outer = data[5] & 0x0F;
    modulation = data[6];
    symbol_rate = 100 * uint64_t(DecodeBCD(data + 7, 7, true));  // coded in 100 sym/s units.
    FEC_inner = data[10] & 0x0F;
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration ModulationNames({
        {u"16-QAM", 1},
        {u"32-QAM", 2},
        {u"64-QAM", 3},
        {u"128-QAM", 4},
        {u"256-QAM", 5},
    });

    const ts::Enumeration OuterFecNames({
        {u"undefined", 0},
        {u"none", 1},
        {u"RS", 2},
    });

    const ts::Enumeration InnerFecNames({
        {u"undefined", 0},
        {u"1/2", 1},
        {u"2/3", 2},
        {u"3/4", 3},
        {u"5/6", 4},
        {u"7/8", 5},
        {u"8/9", 6},
        {u"3/5", 7},
        {u"4/5", 8},
        {u"9/10", 9},
        {u"none", 15},
    });
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CableDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"frequency", frequency, false);
    root->setIntEnumAttribute(OuterFecNames, u"FEC_outer", FEC_outer);
    root->setIntEnumAttribute(ModulationNames, u"modulation", modulation);
    root->setIntAttribute(u"symbol_rate", symbol_rate, false);
    root->setIntEnumAttribute(InnerFecNames, u"FEC_inner", FEC_inner);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CableDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint64_t>(frequency, u"frequency", true) &&
           element->getIntEnumAttribute<uint8_t>(FEC_outer, OuterFecNames, u"FEC_outer", false, 2) &&
           element->getIntEnumAttribute<uint8_t>(modulation, ModulationNames, u"modulation", false, 1) &&
           element->getIntAttribute<uint64_t>(symbol_rate, u"symbol_rate", true) &&
           element->getIntEnumAttribute(FEC_inner, InnerFecNames, u"FEC_inner", true);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CableDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 11) {
        uint8_t fec_outer = data[5] & 0x0F;
        uint8_t modulation = data[6];
        uint8_t fec_inner = data[10] & 0x0F;
        std::string freq, srate;
        BCDToString(freq, data, 8, 4);
        BCDToString(srate, data + 7, 7, 3, true);
        data += 11; size -= 11;

        strm << margin << "Frequency: " << freq << " MHz" << std::endl
             << margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl
             << margin << "Modulation: ";
        switch (modulation) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "16-QAM"; break;
            case 2:  strm << "32-QAM"; break;
            case 3:  strm << "64-QAM"; break;
            case 4:  strm << "128-QAM"; break;
            case 5:  strm << "256-QAM"; break;
            default: strm << "code " << int(modulation) << " (reserved)"; break;
        }
        strm << std::endl << margin << "Outer FEC: ";
        switch (fec_outer) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "none"; break;
            case 2:  strm << "RS(204/188)"; break;
            default: strm << "code " << int(fec_outer) << " (reserved)"; break;
        }
        strm << ", Inner FEC: ";
        switch (fec_inner) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "1/2 conv. code rate"; break;
            case 2:  strm << "2/3 conv. code rate"; break;
            case 3:  strm << "3/4 conv. code rate"; break;
            case 4:  strm << "5/6 conv. code rate"; break;
            case 5:  strm << "7/8 conv. code rate"; break;
            case 6:  strm << "8/9 conv. code rate"; break;
            case 7:  strm << "3/5 conv. code rate"; break;
            case 8:  strm << "4/5 conv. code rate"; break;
            case 9:  strm << "9/10 conv. code rate"; break;
            case 15: strm << "none"; break;
            default: strm << "code " << int(fec_inner) << " (reserved)"; break;
        }
        strm << std::endl;
    }

    display.displayExtraData(data, size, indent);
}
