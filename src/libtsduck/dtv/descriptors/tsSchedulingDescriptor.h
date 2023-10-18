//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an scheduling_descriptor (UNT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an scheduling_descriptor (UNT specific).
    //!
    //! This descriptor cannot be present in other tables than a UNT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 006, 9.5.2.9
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SchedulingDescriptor : public AbstractDescriptor
    {
    public:
        // SchedulingDescriptor public members:
        Time      start_date_time {};             //!< Start time.
        Time      end_date_time {};               //!< End time.
        bool      final_availability = false;     //!< Last schedule.
        bool      periodicity = false;            //!< Periodically available.
        uint8_t   period_unit = 0;                //!< 2 bits, unit of @a period.
        uint8_t   duration_unit = 0;              //!< 2 bits, unit of @a duration.
        uint8_t   estimated_cycle_time_unit = 0;  //!< 2 bits, unit of @a estimated.
        uint8_t   period = 0;                     //!< SSU repetition period.
        uint8_t   duration = 0;                   //!< SSU duration.
        uint8_t   estimated_cycle_time = 0;       //!< Duration of one cycle.
        ByteBlock private_data {};                //!< Private data

        //!
        //! Default constructor.
        //!
        SchedulingDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SchedulingDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        static const Enumeration SchedulingUnitNames;
    };
}
