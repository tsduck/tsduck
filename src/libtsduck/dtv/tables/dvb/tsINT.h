//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an IP/MAC Notification Table (INT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an IP/MAC Notification Table (INT).
    //! @see ETSI EN 301 192, 8.4.3
    //! @ingroup table
    //!
    class TSDUCKDLL INT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a device.
        //!
        class TSDUCKDLL Device : public EntryBase
        {
        public:
            DescriptorList target_descs;       //!< Target descriptor loop, describes the target device.
            DescriptorList operational_descs;  //!< Operational descriptor loop, describes the operations on the target device.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit Device(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            Device(const AbstractTable* table, const Device& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            Device& operator=(const Device& other);

        private:
            // Inaccessible operations.
            Device() = delete;
            Device(const Device&) = delete;
        };

        //!
        //! List of devices.
        //!
        typedef EntryWithDescriptorsList<Device> DeviceList;

        // INT public members:
        uint8_t        action_type = 0;       //!< Action type.
        uint32_t       platform_id = 0;       //!< Platform id, 24 bits.
        uint8_t        processing_order = 0;  //!< Processing order code.
        DescriptorList platform_descs;        //!< Platforma descriptor loop.
        DeviceList     devices;               //!< List of device descriptions.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        INT(uint8_t version = 0, bool is_current = true);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        INT(const INT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        INT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
