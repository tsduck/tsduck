//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    //! @see ETSI EN 300 468, 5.2.3
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
        class TSDUCKDLL ServiceEntry : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(ServiceEntry);
            TS_DEFAULT_ASSIGMENTS(ServiceEntry);
        public:
            // Public members
            bool    EITs_present = false;    //!< There are EIT schedule on current TS.
            bool    EITpf_present = false;   //!< There are EIT present/following on current TS.
            uint8_t running_status = 0;      //!< Running status code.
            bool    CA_controlled = false;   //!< Controlled by a CA_system.

            //!
            //! Constructor.
            //! @param [in] table Parent SDT.
            //!
            ServiceEntry(const AbstractTable* table);

            //!
            //! Get the service type.
            //! @param [in,out] duck TSDuck execution context.
            //! @return The service type, as found from the first DVB "service descriptor", if there is one in the list.
            //! Return zero if there is no service descriptor.
            //!
            uint8_t serviceType(DuckContext& duck) const;

            //!
            //! Get the service name.
            //! @param [in,out] duck TSDuck execution context.
            //! @return The service name, as found from the first DVB "service descriptor", if there is one in the list.
            //!
            UString serviceName(DuckContext& duck) const;

            //!
            //! Get the provider name.
            //! @param [in,out] duck TSDuck execution context.
            //! @return The provider name, as found from the first DVB "service descriptor", if there is one in the list.
            //!
            UString providerName(DuckContext& duck) const;

            //!
            //! Set the service name.
            //!
            //! Modify the first service_descriptor, if there is one,
            //! with the new service name. If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //!
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in] name New service name.
            //! @param [in] service_type If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //! The default service_type is 1, ie. "digital television service".
            //! Ignored if a service_descriptor already exists.
            //!
            void setName(DuckContext& duck, const UString& name, uint8_t service_type = 1)
            {
                setString(duck, &ServiceDescriptor::service_name, name, service_type);
            }

            //!
            //! Set the provider name.
            //!
            //! Modify the first service_descriptor, if there is one,
            //! with the new provider name. If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //!
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in] provider New provider name.
            //! @param [in] service_type If there is no service_descriptor,
            //! a new one is added with the specified @a service type.
            //! The default service_type is 1, ie. "digital television service".
            //! Ignored if a service_descriptor already exists.
            //!
            void setProvider(DuckContext& duck, const UString& provider, uint8_t service_type = 1)
            {
                setString(duck, &ServiceDescriptor::provider_name, provider, service_type);
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
            //! @param [in,out] duck TSDuck execution context.
            //! @param [out] desc Returned content of the service descriptor.
            //! @return True if found and valid, false otherwise.
            //!
            bool locateServiceDescriptor(DuckContext& duck, ServiceDescriptor& desc) const;

            //!
            //! Collect all informations about the service.
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in,out] service A service description to update.
            //!
            void updateService(DuckContext& duck, Service& service) const;

        private:
            //!
            //! Set a string value (typically provider or service name).
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in] field Pointer to UString member in service descriptor.
            //! @param [in] value New string value.
            //! @param [in] service_type If there is no service_descriptor, a new one is added with the specified @a service type.
            //!
            void setString(DuckContext& duck, UString ServiceDescriptor::* field, const UString& value, uint8_t service_type);
        };

        //!
        //! List of services, indexed by service_id.
        //!
        typedef EntryWithDescriptorsMap<uint16_t, ServiceEntry> ServiceMap;

        // SDT public members:
        uint16_t   ts_id = 0;     //!< Transport stream_id.
        uint16_t   onetw_id = 0;  //!< Original network id.
        ServiceMap services;      //!< Map of services: key=service_id, value=service_description.

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
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SDT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SDT(const SDT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        SDT& operator=(const SDT& other) = default;

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
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] name Service name to search.
        //! @param [out] service_id Returned service id.
        //! @param [in] exact_match If true, the service name must be exactly
        //! identical to @a name. If it is false, the search is case-insensitive
        //! and blanks are ignored.
        //! @return True if the service is found, false if not found.
        //!
        bool findService(DuckContext& duck, const UString& name, uint16_t& service_id, bool exact_match = false) const;

        //!
        //! Search a service by name, using a Service class.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] service Service description. Use service name to search.
        //! Set the service id if found.
        //! @param [in] exact_match If true, the service name must be exactly
        //! identical to the name in @a service. If it is false, the search is case-insensitive
        //! and blanks are ignored.
        //! @return True if the service is found, false if not found.
        //!
        bool findService(DuckContext& duck, Service& service, bool exact_match = false) const;

        //!
        //! Collect all informations about all services in the SDT.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] services A list of service descriptions. Existing services
        //! are updated with the informations from the SDT. New entries are created for
        //! other services.
        //!
        void updateServices(DuckContext& duck, ServiceList& services) const;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual bool isValidTableId(TID) const override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
