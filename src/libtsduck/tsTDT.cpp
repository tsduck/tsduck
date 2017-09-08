//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Representation of a Time & Date Table (TDT)
//
//----------------------------------------------------------------------------

#include "tsTDT.h"
#include "tsMJD.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_TABLE_FACTORY(ts::TDT, "TDT");
TS_ID_TABLE_FACTORY(ts::TDT, ts::TID_TDT);
TS_ID_SECTION_DISPLAY(ts::TDT::DisplaySection, ts::TID_TDT);


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::TDT::TDT(const Time& utc_time_) :
    AbstractTable(TID_TDT, "TDT"),
    utc_time(utc_time_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::TDT::TDT(const BinaryTable& table) :
    AbstractTable(TID_TDT, "TDT"),
    utc_time()
{
    deserialize(table);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TDT::deserialize(const BinaryTable& table)
{
    // Clear table content
    _is_valid = false;

    // This is a short table, must have only one section
    if (table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect(*table.sectionAt(0));
    const uint8_t* data(sect.payload());
    size_t remain(sect.payloadSize());

    // Abort if not a TDT
    if (sect.tableId() != TID_TDT || remain < MJD_SIZE) {
        return;
    }

    // Get UTC time
    DecodeMJD(data, MJD_SIZE, utc_time);
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TDT::serialize(BinaryTable& table) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Encode the data in MJD in the payload (5 bytes)
    uint8_t payload[MJD_SIZE];
    EncodeMJD(utc_time, payload, MJD_SIZE);

    // Add the section in the table
    table.addSection(new Section(TID_TDT, // tid
                                 true,    // is_private_section
                                 payload,
                                 MJD_SIZE));
}


//----------------------------------------------------------------------------
// A static method to display a TDT section.
//----------------------------------------------------------------------------

void ts::TDT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 5) {
        Time time;
        DecodeMJD(data, 5, time);
        data += 5; size -= 5;
        strm << std::string(indent, ' ') << "UTC time: "
             << time.format(Time::DATE | Time::TIME) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::TDT::toXML(XML& xml, XML::Element* parent) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TDT::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
