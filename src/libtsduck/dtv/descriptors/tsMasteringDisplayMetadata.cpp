//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of Mastering Display Metadata
//!
//----------------------------------------------------------------------------

#include "tsMasteringDisplayMetadata.h"

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::Mastering_Display_Metadata_type::Mastering_Display_Metadata_type()
{
    clearContent();
}

void ts::Mastering_Display_Metadata_type::clearContent()
{
    X_c0 = 0;
    Y_c0 = 0;
    X_c1 = 0;
    Y_c1 = 0;
    X_c2 = 0;
    Y_c2 = 0;
    X_wp = 0;
    Y_wp = 0;
    L_max = 0;
    L_min = 0;
    MaxCLL = 0;
    MaxFALL = 0;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::Mastering_Display_Metadata_type::deserialize(PSIBuffer& buf)
{
    X_c0 = buf.getUInt16();
    Y_c0 = buf.getUInt16();
    X_c1 = buf.getUInt16();
    Y_c1 = buf.getUInt16();
    X_c2 = buf.getUInt16();
    Y_c2 = buf.getUInt16();
    X_wp = buf.getUInt16();
    Y_wp = buf.getUInt16();
    L_max = buf.getUInt32();
    L_min = buf.getUInt32();
    MaxCLL = buf.getUInt16();
    MaxFALL = buf.getUInt16();
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::Mastering_Display_Metadata_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(X_c0);
    buf.putUInt16(Y_c0);
    buf.putUInt16(X_c1);
    buf.putUInt16(Y_c1);
    buf.putUInt16(X_c2);
    buf.putUInt16(Y_c2);
    buf.putUInt16(X_wp);
    buf.putUInt16(Y_wp);
    buf.putUInt32(L_max);
    buf.putUInt32(L_min);
    buf.putUInt16(MaxCLL);
    buf.putUInt16(MaxFALL);
}


//----------------------------------------------------------------------------
// Static method to display the Mastering Display Metadata values
//----------------------------------------------------------------------------

void ts::Mastering_Display_Metadata_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Mastering Display Metadata" << std::endl;
    disp << margin << "  Chromaticity coordinates (green) X: " << buf.getUInt16();
    disp << ", Y: " << buf.getUInt16() << std::endl;
    disp << margin << "  Chromaticity coordinates (blue) X: " << buf.getUInt16();
    disp << ", Y: " << buf.getUInt16() << std::endl;
    disp << margin << "  Chromaticity coordinates (red) X: " << buf.getUInt16();
    disp << ", Y: " << buf.getUInt16() << std::endl;
    disp << margin << "  Chromaticity coordinates (white point) X: " << buf.getUInt16();
    disp << ", Y: " << buf.getUInt16() << std::endl;
    disp << margin << "  Luminance max: " << buf.getUInt32();
    disp << ", min: " << buf.getUInt32() << std::endl;
    disp << margin << "  Max Content Light Level: " << buf.getUInt16();
    disp << ", Max Frame Average Light Level: " << buf.getUInt16() << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::Mastering_Display_Metadata_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"X_c0", X_c0, false);
    root->setIntAttribute(u"Y_c0", Y_c0, false);
    root->setIntAttribute(u"X_c1", X_c1, false);
    root->setIntAttribute(u"Y_c1", Y_c1, false);
    root->setIntAttribute(u"X_c2", X_c2, false);
    root->setIntAttribute(u"Y_c2", Y_c2, false);
    root->setIntAttribute(u"X_wp", X_wp, false);
    root->setIntAttribute(u"Y_wp", Y_wp, false);
    root->setIntAttribute(u"L_max", L_max, false);
    root->setIntAttribute(u"L_min", L_min, false);
    root->setIntAttribute(u"MaxCLL", MaxCLL, false);
    root->setIntAttribute(u"MaxFALL", MaxFALL, false);
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::Mastering_Display_Metadata_type::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(X_c0, u"X_c0", true) &&
           element->getIntAttribute(Y_c0, u"Y_c0", true) &&
           element->getIntAttribute(X_c1, u"X_c1", true) &&
           element->getIntAttribute(Y_c1, u"Y_c1", true) &&
           element->getIntAttribute(X_c2, u"X_c2", true) &&
           element->getIntAttribute(Y_c2, u"Y_c2", true) &&
           element->getIntAttribute(X_wp, u"X_wp", true) &&
           element->getIntAttribute(Y_wp, u"Y_wp", true) &&
           element->getIntAttribute(L_max, u"L_max", true) &&
           element->getIntAttribute(L_min, u"L_min", true) &&
           element->getIntAttribute(MaxCLL, u"MaxCLL", true) &&
           element->getIntAttribute(MaxFALL, u"MaxFALL", true);
}
