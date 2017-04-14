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
//  Representation of a Service Description Table (SDT)
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsService.h"

namespace ts {

    class TSDUCKDLL SDT : public AbstractLongTable
    {
    public:
        // List of services, indexed by service_id
        class Service;
        typedef std::map <uint16_t, Service> ServiceMap;

        // SDT public members:
        uint16_t     ts_id;     // transport_stream_id
        uint16_t     onetw_id;  // original_network_id
        ServiceMap services;  // key=service_id, value=service_description

        // Default constructor:
        SDT (bool is_actual_ = true,
             uint8_t version_ = 0,
             bool is_current_ = true,
             uint16_t ts_id_ = 0,
             uint16_t onetw_id_ = 0);

        // Constructor from a binary table
        SDT (const BinaryTable& table);

        // Check/set if this is an "actual" SDT.
        // True means "SDT Actual", false means "SDT Other"
        bool isActual () const {return _table_id == TID_SDT_ACT;}
        void setActual (bool is_actual) {_table_id = TID(is_actual ? TID_SDT_ACT : TID_SDT_OTH);}

        // Search a service by name.
        // If the service is found, return true and set service_id. Return
        // false if not found.
        // If exact_match is true, the service name must be exactly identical
        // to name. If it is false, the search is case-insensitive and blanks
        // are ignored.
        bool findService (const std::string& name, uint16_t& service_id, bool exact_match = false) const;

        // Same as previous, but use service name and set service id.
        bool findService (ts::Service&, bool exact_match = false) const;

        // Inherited methods
        virtual void serialize (BinaryTable& table) const;
        virtual void deserialize (const BinaryTable& table);

        // Description of a service
        class TSDUCKDLL Service
        {
        public:
            // Public members
            bool           EITs_present;    // EITs on current TS
            bool           EITpf_present;   // EITp/f on current TS
            uint8_t          running_status;  // running status code
            bool           CA_controlled;   // controlled by a CA_system
            DescriptorList descs;           // descriptor list

            // Default constructor:
            Service();

            // Return the service type, service name and provider name
            // (all found from the first DVB "service descriptor", if
            // there is one in the list).
            uint8_t serviceType() const;
            std::string serviceName() const;
            std::string providerName() const;

            // Modify the first service_descriptor, if there is one,
            // with the new service name. If there is no service_descriptor,
            // a new one is added with the specified service type.
            // The default service_type is 1, ie. "digital television
            // service". If a service_descriptor already exists,
            // service_type is ignored.
            void setName (const std::string& name, uint8_t service_type = 1);

            // Modify the first service_descriptor, if there is one,
            // with the new provider name. If there is no service_descriptor,
            // a new one is added with the specified service type.
            // The default service_type is 1, ie. "digital television
            // service". If a service_descriptor already exists,
            // service_type is ignored.
            void setProvider (const std::string& provider, uint8_t service_type = 1);

            // Modify the first service_descriptor, if there is one,
            // with the new service type. If there is no service_descriptor,
            // a new one is added with the specified service type and empty
            // provider and service names.
            void setType (uint8_t service_type);
        };

    private:
        // Add a new section to a table being serialized
        // Session number is incremented. Data and remain are reinitialized.
        void addSection (BinaryTable& table,
                         int& section_number,
                         uint8_t* payload,
                         uint8_t*& data,
                         size_t& remain) const;
    };
}
