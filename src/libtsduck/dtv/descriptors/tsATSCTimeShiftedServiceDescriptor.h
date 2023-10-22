//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC time_shifted_service_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ATSC time_shifted_service_descriptor.
    //! @see ATSC A/65, section 6.9.6.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ATSCTimeShiftedServiceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t time_shift = 0;            //!< 10 bits, time shift in minutes.
            uint16_t major_channel_number = 0;  //!< 10 bits, major channel of time-shifted service.
            uint16_t minor_channel_number = 0;  //!< 10 bits, minor channel of time-shifted service.

            //!
            //! Default constructor.
            //! @param [in] min Time shift in minutes.
            //! @param [in] major Major channel of time-shifted service.
            //! @param [in] minor Minor channel of time-shifted service.
            //!
            Entry(uint16_t min = 0, uint16_t major = 0, uint16_t minor = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit the count on 5 bits.
        //!
        static constexpr size_t MAX_ENTRIES = 31;

        // Public members:
        EntryList entries {};  //!< The list of service entries.

        //!
        //! Default constructor.
        //!
        ATSCTimeShiftedServiceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCTimeShiftedServiceDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
