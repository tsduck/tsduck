//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Data Service Table (DST)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsDSMCCCompatibilityDescriptor.h"
#include "tsDSMCCTap.h"

namespace ts {
    //!
    //! Representation of an ATSC Data Service Table (DST).
    //! @see ATSC A/90, section 12.2.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL DST : public AbstractLongTable
    {
    public:
        //!
        //! Description of a tap.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Tap : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Tap);
            TS_DEFAULT_ASSIGMENTS(Tap);
        public:
            // Public members
            uint8_t  protocol_encapsulation = 0;  //!< Protocol encapsulation.
            uint8_t  action_type = 0;             //!< 7 bits, action type.
            bool     resource_location = false;   //!< If false, association_tag in PMT. If true, it is in a DSM-CC Resource Descriptor within NRT.
            DSMCCTap tap {};                      //!< DSM-CC Tap() structure.

            //!
            //! Constructor.
            //! @param [in] table Parent DST.
            //!
            Tap(const AbstractTable* table);
        };

        //!
        //! List of taps.
        //!
        using TapList = AttachedEntryList<Tap>;

        //!
        //! Description of an application.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Application : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Application);
            TS_DEFAULT_ASSIGMENTS(Application);
        public:
            // Public members
            DSMCCCompatibilityDescriptor compatibility_descriptor {};  //!< DSM-CC compatibilityDescriptor() structure.
            std::optional<uint16_t>      app_id_description {};        //!< Optional app_id.
            ByteBlock                    app_id {};                    //!< Meaningful only if app_id_description has a value.
            TapList                      taps;                         //!< List of taps.
            ByteBlock                    app_data {};                  //!< Application data.

            //!
            //! Constructor.
            //! @param [in] table Parent DST.
            //!
            Application(const AbstractTable* table);
        };

        //!
        //! List of applications.
        //!
        using ApplicationList = AttachedEntryList<Application>;

        // DST public members:
        uint16_t        table_id_extension = 0xFFFF;  //!< ATSC reserved.
        uint8_t         sdf_protocol_version = 1;     //!< ATSC SDF protocol version.
        ApplicationList apps;                         //!< List of applications.
        DescriptorList  descs;                        //!< Service information descriptors loop.
        ByteBlock       service_private_data {};      //!< Service private data.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        DST(uint8_t version = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DST(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        DST(const DST& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        DST& operator=(const DST& other) = default;

        // Inherited methods
        virtual bool isCurrent() const override;
        virtual void setCurrent(bool is_current) override;
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

    private:
        // In a DST, current is always true.
        static constexpr bool CURRENT = true;
    };
}
