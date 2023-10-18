//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Time Offset Table (TOT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsDescriptorList.h"
#include "tsLocalTimeOffsetDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a Time Offset Table (TOT)
    //! @see ETSI EN 300 468, 5.2.6
    //! @ingroup table
    //!
    class TSDUCKDLL TOT : public AbstractTable
    {
    public:
        typedef LocalTimeOffsetDescriptor::Region       Region;        //!< Alias from LocalTimeOffsetDescriptor.
        typedef LocalTimeOffsetDescriptor::RegionVector RegionVector;  //!< Alias from LocalTimeOffsetDescriptor.

        // Public members:
        Time           utc_time {}; //!< UTC time.
        RegionVector   regions {};  //!< Vector of region descriptions.
        DescriptorList descs;       //!< Descriptor list, except local_time_offset_descriptor.

        //!
        //! Default constructor.
        //! @param [in] utc_time UTC time.
        //!
        TOT(const Time& utc_time = Time::Epoch);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        TOT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        TOT(const TOT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        TOT& operator=(const TOT& other) = default;

        //!
        //! Get the local time according to a region description.
        //! Use the UTC time from the TOT and the local time offset from the region.
        //! @param [in] region Region to get the local time for.
        //! @return The local time for @a region.
        //!
        Time localTime(const Region& region) const;

        //!
        //! Format a time offset string.
        //! @param minutes A time offset in minutes.
        //! @return A string like "+hh:mm" or "-hh:mm".
        //!
        static UString timeOffsetFormat(int minutes);

        // Inherited methods
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual bool useTrailingCRC32() const override;
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Add descriptors, filling regions from local_time_offset_descriptor's.
        void addDescriptors(DuckContext& duck, const DescriptorList& dlist);

        // Last encountered offset from UTC in the context, typically for ISDB.
        mutable MilliSecond _time_reference_offset;
    };
}
