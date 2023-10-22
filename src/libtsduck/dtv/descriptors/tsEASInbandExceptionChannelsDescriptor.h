//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 18 EAS_inband_exception_channels_descriptor
//!  (specific to a Cable Emergency Alert Table).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SCTE 18 EAS_inband_exception_channels_descriptor (specific to a Cable Emergency Alert Table).
    //!
    //! This descriptor cannot be present in other tables than a Cable Emergency Alert Table).
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 18, 5.1.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EASInbandExceptionChannelsDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Exception entry.
        //!
        class TSDUCKDLL Entry
        {
        public:
            // Public members
            uint8_t  RF_channel = 0;      //!< RF channel number of the carrier.
            uint16_t program_number = 0;  //!< Programe number, aka service id.

            //!
            //! Default constructor.
            //! @param [in] chan RF channel number of the carrier.
            //! @param [in] prog Programe number, aka service id.
            //!
            Entry(uint8_t chan = 0, uint16_t prog = 0);
        };

        //!
        //! List of exception entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 254 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 84;

        // EASInbandExceptionChannelsDescriptor public members:
        EntryList entries {};  //!< The list of exception entries.

        //!
        //! Default constructor.
        //!
        EASInbandExceptionChannelsDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EASInbandExceptionChannelsDescriptor(DuckContext& duck, const Descriptor& bin);

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
