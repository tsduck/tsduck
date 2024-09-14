//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
        DeliverySystem _system = DS_UNDEFINED;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] sys The delivery system.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //!
        AbstractDeliverySystemDescriptor(DID tag, DeliverySystem sys, const UChar* xml_name, Standards standards = Standards::DVB, PDS pds = 0);

        //!
        //! Generic tool to translate an integer value from a binary descriptor into a modulation parameter.
        //! @tparam ENUM The integer or enumeration type for the modulation parameter.
        //! @param value The integer value from the binary descriptor.
        //! @param definition A map of all supported values.
        //! @param defvalue The default value to return if @a value is not a supported one.
        //! @return The corresponding modulation parameter.
        //!
        template <typename ENUM, typename std::enable_if<std::is_integral<ENUM>::value || std::is_enum<ENUM>::value>::type* = nullptr>
        static ENUM translate(int value, const std::map<int,ENUM>& definition, ENUM defvalue);

    private:
        // Unreachable constructors and operators.
        AbstractDeliverySystemDescriptor() = delete;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template <typename ENUM, typename std::enable_if<std::is_integral<ENUM>::value || std::is_enum<ENUM>::value>::type*>
ENUM ts::AbstractDeliverySystemDescriptor::translate(int value, const std::map<int,ENUM>& definition, ENUM defvalue)
{
    const auto it = definition.find(value);
    return it == definition.end() ? defvalue : it->second;
}
