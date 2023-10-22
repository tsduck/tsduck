//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC System Time Table (STT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ATSC System Time Table (STT)
    //! @see ATSC A/65, section 6.1.
    //! @ingroup table
    //!
    //! Warning: The ATSC STT violates the common usage rules of MPEG sections,
    //! as defined in ISO/IEC 13818-1. An STT section is, by essence, unique.
    //! Each section carries a different system time. According the MPEG rules,
    //! this should be a short section, just like a TDT or TOT, its DVB counterparts.
    //! However, ATSC defines the STT as a long section with version zero.
    //! Normally, all consecutive sections with same tid, tid ext and version
    //! should be identical. But this is not the case with the ATSC STT.
    //! Specifically, when a SectionDemux is used, it reports only the first
    //! occurence of the STT because all subsequent sections have the same version.
    //! To be notified of all STT sections with a SectionDemux, it is necessary
    //! to provide a section handler, not a table handler.
    //!
    class TSDUCKDLL STT : public AbstractLongTable
    {
    public:
        // STT public members:
        uint8_t        protocol_version = 0;  //!< ATSC protocol version.
        uint32_t       system_time = 0;       //!< Number of GPS seconds since 00:00:00 UTC, January 6th, 1980.
        uint8_t        GPS_UTC_offset = 0;    //!< Current offset in seconds between GPS and UTC (UTC = GPS - GPS_UTC_offset)
        bool           DS_status = false;     //!< Daylight Saving time is on.
        uint8_t        DS_day_of_month = 0;   //!< Day of month (1-31) for DS transition, zero if none planned in the next 28 days.
        uint8_t        DS_hour = 0;           //!< Hour of day for next DS transition, zero if none planned.
        DescriptorList descs;                 //!< Descriptor list.

        //!
        //! Default constructor.
        //!
        STT();

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        STT(const STT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        STT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Constructor from a binary section.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] section Binary section to deserialize.
        //!
        STT(DuckContext& duck, const Section& section);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        STT& operator=(const STT& other) = default;

        //!
        //! Convert the GPS system time in this object in a UTC time.
        //! @return The system time as a UTC time or Time::Epoch if unset..
        //!
        Time utcTime() const;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
