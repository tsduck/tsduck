//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDeliverySystemDescriptor.h"
#include "tsDescriptor.h"

ts::AbstractDeliverySystemDescriptor::AbstractDeliverySystemDescriptor(DID tag, DeliverySystem sys, const UChar* xml_name) :
    AbstractDescriptor(tag, xml_name, Standards::DVB, 0),
    _system(sys)
{
}

ts::AbstractDeliverySystemDescriptor::~AbstractDeliverySystemDescriptor()
{
}

ts::DeliverySystem ts::AbstractDeliverySystemDescriptor::deliverySystem(const DuckContext& duck) const
{
    // Default implementation.
    return _system;
}

ts::DescriptorDuplication ts::AbstractDeliverySystemDescriptor::duplicationMode() const
{
    // By default, there is only one system delivery descriptor per descriptor list.
    // Adding a new one replaces the previous one.
    return DescriptorDuplication::REPLACE;
}
