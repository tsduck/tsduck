//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPERealTimeParameters.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of the structure.
//----------------------------------------------------------------------------

void ts::MPERealTimeParameters::clearContent()
{
    delta_t = 0;
    table_boundary = false;
    frame_boundary = false;
    address = 0;
}


//----------------------------------------------------------------------------
// Binary serialization.
//----------------------------------------------------------------------------

void ts::MPERealTimeParameters::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(delta_t, 12);
    buf.putBit(table_boundary);
    buf.putBit(frame_boundary);
    buf.putBits(address, 18);
}


void ts::MPERealTimeParameters::deserialize(PSIBuffer& buf)
{
    buf.getBits(delta_t, 12);
    table_boundary = buf.getBool();
    frame_boundary = buf.getBool();
    buf.getBits(address, 18);
}


//----------------------------------------------------------------------------
// Field names, index #0: ETSI EN 301 192, index #1: ETSI TS 102 772
//----------------------------------------------------------------------------

namespace {
    const ts::UChar* const table_boundary_name[] = {u"table_boundary", u"MPE_boundary"};
    const ts::UChar* const address_name[] = {u"address", u"prev_burst_size"};
}


//----------------------------------------------------------------------------
// XML conversions.
//----------------------------------------------------------------------------

ts::xml::Element* ts::MPERealTimeParameters::buildXML(DuckContext& duck, xml::Element* parent, bool use_etsi_ts_102_772_names, const UString& element_name) const
{
    xml::Element* element = parent->addElement(element_name);
    element->setIntAttribute(u"delta_t", delta_t);
    element->setBoolAttribute(table_boundary_name[use_etsi_ts_102_772_names], table_boundary);
    element->setBoolAttribute(u"frame_boundary", frame_boundary);
    element->setIntAttribute(address_name[use_etsi_ts_102_772_names], address);
    return element;
}

bool ts::MPERealTimeParameters::analyzeXML(DuckContext& duck, const xml::Element* parent, bool use_etsi_ts_102_772_names, const UString& element_name)
{
    xml::ElementVector xe;
    return parent->getChildren(xe, element_name, 1, 1) &&
           xe[0]->getIntAttribute(delta_t, u"delta_t", true, 0, 0, 0x0FFF) &&
           xe[0]->getBoolAttribute(table_boundary, table_boundary_name[use_etsi_ts_102_772_names], true) &&
           xe[0]->getBoolAttribute(frame_boundary, u"frame_boundary", true) &&
           xe[0]->getIntAttribute(address, address_name[use_etsi_ts_102_772_names], true, 0, 0, 0x03'FFFF);
}


//----------------------------------------------------------------------------
// A static method to display a real_time_parameters structure.
//----------------------------------------------------------------------------

bool ts::MPERealTimeParameters::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, bool use_etsi_ts_102_772_names)
{
    const bool ok = buf.canReadBits(32);
    if (ok) {
        disp << margin << "- Real time parameters:" << std::endl;
        disp << margin << UString::Format(u"  delta_t: %n", buf.getBits<uint16_t>(12)) << std::endl;
        disp << margin << UString::Format(u"  %s: %s", table_boundary_name[use_etsi_ts_102_772_names], buf.getBool()) << std::endl;
        disp << margin << UString::Format(u"  frame_boundary: %s", buf.getBool()) << std::endl;
        disp << margin << UString::Format(u"  %s: %n", address_name[use_etsi_ts_102_772_names], buf.getBits<uint32_t>(18)) << std::endl;
    }
    return ok;
}
