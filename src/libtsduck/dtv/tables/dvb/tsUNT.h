//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Update Notification Table (UNT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsDSMCCCompatibilityDescriptor.h"

namespace ts {
    //!
    //! Representation of an Update Notification Table (INT).
    //! @see ETSI TS 102 006, 9.4.1
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL UNT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a platform.
        //!
        class TSDUCKDLL Platform : public AttachedEntry
        {
        public:
            DescriptorList target_descs;       //!< Target descriptor loop, describes the target platform.
            DescriptorList operational_descs;  //!< Operational descriptor loop, describes the operations on the target platform.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit Platform(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            Platform(const AbstractTable* table, const Platform& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            Platform& operator=(const Platform& other) = default;

        private:
            // Inaccessible operations.
            Platform() = delete;
            Platform(const Platform&) = delete;
        };

        //!
        //! List of platforms.
        //!
        using PlatformList = AttachedEntryList<Platform>;

        //!
        //! Description of a set of devices.
        //!
        class TSDUCKDLL Devices : public AttachedEntry
        {
        public:
            DSMCCCompatibilityDescriptor compatibilityDescriptor {};   //!< The entries of the compatibilityDescriptor.
            PlatformList                 platforms;                    //!< The list of platforms.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit Devices(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            Devices(const AbstractTable* table, const Devices& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            Devices& operator=(const Devices& other) = default;

        private:
            // Inaccessible operations.
            Devices() = delete;
            Devices(const Devices&) = delete;
        };

        //!
        //! List of devicess.
        //!
        using DevicesList = AttachedEntryList<Devices>;

        // UNT public members:
        uint8_t        action_type = 0;       //!< Action type.
        uint32_t       OUI = 0;               //!< OUI, 24 bits.
        uint8_t        processing_order = 0;  //!< Processing order code.
        DescriptorList descs;                 //!< Common descriptor loop.
        DevicesList    devices;               //!< List of sets of devices.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        UNT(uint8_t version = 0, bool is_current = true);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        UNT(const UNT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        UNT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        virtual DescriptorList* topLevelDescriptorList() override;
        virtual const DescriptorList* topLevelDescriptorList() const override;
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
