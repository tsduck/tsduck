//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//
//  Encapsulation of Linux S2API (new DVB API) property lists.
//
//-----------------------------------------------------------------------------

#pragma once
#include "tsReportInterface.h"

#if defined (__s2api)

namespace ts {

    class TSDUCKDLL DTVProperties
    {
    public:
        // Constructor / destructor
        DTVProperties();
        virtual ~DTVProperties();

        // Get the number of properties in the buffer.
        size_t count() const {return size_t (_prop_head.num);}

        // Add a new property. Return index in property buffer.
        size_t add (uint32_t cmd, uint32_t data = -1);

        // Search a property in the buffer, return index in buffer or count() if not found.
        size_t search (uint32_t cmd) const;

        // Get the value of a property in the buffer or UNKNOWN if not found
        uint32_t getByCommand (uint32_t cmd) const;

        // Get the value of the property at specified index or UNKNOWN if out of range
        uint32_t getByIndex (size_t index) const {return index >= size_t (_prop_head.num) ? UNKNOWN : _prop_buffer[index].u.data;}

        // Get the address of the dtv_properties structure for ioctl() call
        const ::dtv_properties* getIoctlParam() const {return &_prop_head;}
        ::dtv_properties* getIoctlParam() {return &_prop_head;}

        // Returned value for unknown data
        static const uint32_t UNKNOWN = ~0;

        // Report the content of the object (for debug purpose)
        void report (ReportInterface&, int severity) const;

        // Return the name of a S2API command or zero if unknown
        static const char* CommandNameS2API (uint32_t cmd);

    private:
        // Private members:
        ::dtv_property _prop_buffer [DTV_IOCTL_MAX_MSGS];
        ::dtv_properties _prop_head;
    };
}

#endif // __s2api
