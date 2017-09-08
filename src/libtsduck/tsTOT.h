//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a Time Offset Table (TOT)
    //!
    class TSDUCKDLL TOT : public AbstractTable
    {
    public:
        //!
        //! Description of one region.
        //!
        struct TSDUCKDLL Region
        {
            std::string  country;            //!< Country code, must be 3-chars long.
            unsigned int region_id;          //!< Region id.
            int          time_offset;        //!< Local time minus UTC, in minutes.
            Time         next_change;        //!< UTC of next time change.
            int          next_time_offset;   //!< Time @ time_offset after @a next_change.

            //!
            //! Default constructor.
            //!
            Region() :
                country(),
                region_id(0),
                time_offset(0),
                next_change(),
                next_time_offset(0)
            {
            }
        };

        //!
        //! Vector of region descriptions.
        //!
        typedef std::vector<Region> RegionVector;

        // Public members:
        Time           utc_time; //!< UTC time.
        RegionVector   regions;  //!< Vector of region descriptions.
        DescriptorList descs;    //!< Descriptor list, except local_time_offset_descriptor.

        //!
        //! Default constructor.
        //! @param [in] utc_time UTC time.
        //!
        TOT(const Time& utc_time = Time::Epoch);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //!
        TOT(const BinaryTable& table);

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
        static std::string timeOffsetFormat(int minutes);

        // Inherited methods
        virtual void serialize(BinaryTable& table) const;
        virtual void deserialize(const BinaryTable& table);
        virtual XML::Element* toXML(XML& xml, XML::Element* parent) const;
        virtual void fromXML(XML& xml, const XML::Element* element);

        //!
        //! A static method to display a section.
        //! @param [in,out] display Display engine.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        static void DisplaySection(TablesDisplay& display, const Section& section, int indent);
    };
}
