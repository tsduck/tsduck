//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Representation of a Service Description Table (SDT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsServiceDescriptor.h"
#include "tsService.h"

namespace ts {
    //!
    //! Representation of a Service Description Table (SDT).
    //! @ingroup table
    //!
    class TSDUCKDLL SDT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a service.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Service : public EntryWithDescriptors
        {
        public:
            // Public members
            bool    EITs_present;    //!< There are EIT schedule on current TS.
            bool    EITpf_present;   //!< There are EIT present/following on current TS.
            uint8_t running_status;  //!< Running status code.
            bool    CA_controlled;   //!< Controlled by a CA_system.

            //!
            //! Constructor.
            //! @param [in] table Parent SDT.
            //!
            Service(const AbstractTable* table);

            //!
            //! Get the service type.
            //! @return The service type, as found from the first DVB
            //! "service descriptor", if there is one in the list.
            //!
            uint8_t serviceType() const;

            //!
            //! Get the service name.
            //! @param [in] charset If not zero, character set to use without explicit table code.
            //! @return The service name, as found from the first DVB
            //! "service descriptor", if there is one in the list.
            //!
            UString serviceName(const DVBCharset* charset = nullptr) const;

            //!
            //! Get the provider name.
            //! @param [in] charset If not zero, character set to use without explicit table code.
            //! @return The provider name, as found from the first DVB
            //! "service descriptor", if there is one in the list.
            //!
            UString providerName(const DVBCharset* charset = nullptr) const;

            //!
            //! Set the service name.
            //!
            //! Modify the first service_descriptor, if there is one,
            //! with the new service name. If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //!
            //! @param [in] name New service name.
            //! @param [in] service_type If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //! The default service_type is 1, ie. "digital television service".
            //! Ignored if a service_descriptor already exists.
            //! @param [in] charset If not zero, character set to use for decoding without explicit table code
            //! and preferred character set for DVB encoding.
            //!
            void setName(const UString& name, uint8_t service_type = 1, const DVBCharset* charset = nullptr)
            {
                setString(&ServiceDescriptor::service_name, name, service_type, charset);
            }

            //!
            //! Set the provider name.
            //!
            //! Modify the first service_descriptor, if there is one,
            //! with the new provider name. If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //!
            //! @param [in] provider New provider name.
            //! @param [in] service_type If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //! The default service_type is 1, ie. "digital television service".
            //! Ignored if a service_descriptor already exists.
            //! @param [in] charset If not zero, character set to use for decoding without explicit table code
            //! and preferred character set for DVB encoding.
            //!
            void setProvider(const UString& provider, uint8_t service_type = 1, const DVBCharset* charset = nullptr)
            {
                setString(&ServiceDescriptor::provider_name, provider, service_type, charset);
            }

            //!
            //! Set the service type.
            //!
            //! Modify the first service_descriptor, if there is one,
            //! with the new service type. If there is no service_descriptor,
            //! a new one is added with empty provider and service names.
            //!
            //! @param [in] service_type New service type.
            //!
            void setType(uint8_t service_type);

            //!
            //! Locate and deserialize the first DVB service_descriptor inside the entry.
            //! @param [out] desc Returned content of the service descriptor.
            //! @param [in] charset If not zero, character set to use without explicit table code.
            //! @return True if found and valid, false otherwise.
            //!
            bool locateServiceDescriptor(ServiceDescriptor& desc, const DVBCharset* charset = nullptr) const;

        private:
            //!
            //! Set a string value (typically provider or service name).
            //! @param [in] field Pointer to UString member in service descriptor.
            //! @param [in] value New string value.
            //! @param [in] service_type If there is no service_descriptor, a new one is added with the specified @a service type.
            //! @param [in] charset If not zero, defautl DVB character set to use.
            //!
            void setString(UString ServiceDescriptor::* field, const UString& value, uint8_t service_type, const DVBCharset* charset);

            // Inaccessible operations.
            Service() = delete;
            Service(const Service&) = delete;
        };

        //!
        //! List of services, indexed by service_id.
        //!
        typedef EntryWithDescriptorsMap<uint16_t, Service> ServiceMap;

        // SDT public members:
        uint16_t   ts_id;     //!< Transport stream_id.
        uint16_t   onetw_id;  //!< Original network id.
        ServiceMap services;  //!< Map of services: key=service_id, value=service_description.

        //!
        //! Default constructor.
        //! @param [in] is_actual True for SDT Actual TS, false for SDT Other TS.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] ts_id Transport stream identifier.
        //! @param [in] onetw_id Original network id.
        //!
        SDT(bool is_actual = true,
            uint8_t version = 0,
            bool is_current = true,
            uint16_t ts_id = 0,
            uint16_t onetw_id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        SDT(const BinaryTable& table, const DVBCharset* charset = nullptr);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SDT(const SDT& other);

        //!
        //! Check if this is an "actual" SDT.
        //! @return True for SDT Actual TS, false for SDT Other TS.
        //!
        bool isActual() const
        {
            return _table_id == TID_SDT_ACT;
        }

        //!
        //! Set if this is an "actual" SDT.
        //! @param [in] is_actual True for SDT Actual TS, false for SDT Other TS.
        //!
        void setActual(bool is_actual)
        {
            _table_id = is_actual ? TID_SDT_ACT : TID_SDT_OTH;
        }

        //!
        //! Search a service by name.
        //! @param [in] name Service name to search.
        //! @param [out] service_id Returned service id.
        //! @param [in] exact_match If true, the service name must be exactly
        //! identical to @a name. If it is false, the search is case-insensitive
        //! and blanks are ignored.
        //! @return True if the service is found, false if not found.
        //!
        bool findService(const UString& name, uint16_t& service_id, bool exact_match = false) const;

        //!
        //! Search a service by name, using a ts::Service class.
        //! @param [in,out] service Service description. Use service name to search.
        //! Set the service id if found.
        //! @param [in] exact_match If true, the service name must be exactly
        //! identical to the name in @a service. If it is false, the search is case-insensitive
        //! and blanks are ignored.
        //! @return True if the service is found, false if not found.
        //!
        bool findService(ts::Service& service, bool exact_match = false) const;

        // Inherited methods
        virtual void serialize(BinaryTable& table, const DVBCharset* = nullptr) const override;
        virtual void deserialize(const BinaryTable& table, const DVBCharset* = nullptr) override;
        virtual void buildXML(xml::Element*) const override;
        virtual void fromXML(const xml::Element*) override;
        DeclareDisplaySection();

    private:
        // Add a new section to a table being serialized
        // Section number is incremented. Data and remain are reinitialized.
        void addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const;
    };
}
