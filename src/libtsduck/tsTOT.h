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
//
//  Representation of a Time Offset Table (TOT)
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {

    class TSDUCKDLL TOT : public AbstractTable
    {
    public:
        // Description of one region
        struct TSDUCKDLL Region
        {
            std::string  country;            // Must be 3-chars long
            unsigned int region_id;
            int          time_offset;        // Local - UTC, in minutes
            Time         next_change;        // UTC of next time change
            int          next_time_offset;   // Time offset after next_change

            // Constructor
            Region() : country(), region_id(0), time_offset(0), next_change(), next_time_offset(0) {}
        };
        typedef std::vector <Region> RegionVector;

        // Public members:
        Time           utc_time;
        RegionVector   regions;
        DescriptorList descs;  // Other than local_time_offset_descriptor

        // Default constructor:
        TOT (const Time& utc_time);

        // Constructor from a binary table
        TOT (const BinaryTable& table);

        // Return the local time according to a region description
        Time localTime (const Region&) const;

        // Format a time offset in minutes
        static std::string timeOffsetFormat (int);

        // Inherited methods
        virtual void serialize (BinaryTable& table) const;
        virtual void deserialize (const BinaryTable& table);
    };
}
