//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for DVB delivery system descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsModulationArgs.h"

namespace ts {

    //!
    //! Abstract base class for DVB delivery system descriptors.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AbstractDeliverySystemDescriptor : public AbstractDescriptor
    {
        TS_RULE_OF_FIVE(AbstractDeliverySystemDescriptor, override);
    public:
        //!
        //! Get the delivery system.
        //! @param [in] duck TSDuck execution context.
        //! @return The delivery system.
        //!
        virtual DeliverySystem deliverySystem(const DuckContext& duck) const;

        // Inherited methods.
        virtual DescriptorDuplication duplicationMode() const override;

    protected:
        //!
        //! The delivery system can be modified by subclasses only
        //!
        DeliverySystem _system {DS_UNDEFINED};

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] sys The delivery system.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //!
        AbstractDeliverySystemDescriptor(DID tag, DeliverySystem sys, const UChar* xml_name);

    private:
        // Unreachable constructors and operators.
        AbstractDeliverySystemDescriptor() = delete;
    };
}
