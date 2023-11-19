//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a network_change_notify_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a network_change_notify_descriptor
    //! @see ETSI EN 300 468, 6.4.9.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NetworkChangeNotifyDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Network change entry.
        //!
        struct TSDUCKDLL Change
        {
            Change() = default;                   //!< Default constructor.
            uint8_t  network_change_id = 0;       //!< Network change id.
            uint8_t  network_change_version = 0;  //!< Network change version.
            Time     start_time_of_change {};     //!< Start time of change.
            Second   change_duration = 0;         //!< Change duration in seconds (must be less than 12 hours)?
            uint8_t  receiver_category = 0;       //!< 3 bits, 0 for all, 1 for T2/S2/C2.
            uint8_t  change_type = 0;             //!< 4 bits, type of change.
            uint8_t  message_id = 0;              //!< Message id.
            std::optional<uint16_t> invariant_ts_tsid {};  //!< Optional invariant TS id.
            std::optional<uint16_t> invariant_ts_onid {};  //!< Original network id of optional invariant TS.
        };

        //!
        //! List of change entries.
        //!
        typedef std::list<Change> ChangeList;

        //!
        //! Cell entry.
        //!
        struct TSDUCKDLL Cell
        {
            Cell() = default;        //!< Default constructor.
            uint16_t   cell_id = 0;  //!< Cell id.
            ChangeList changes {};   //!< List of changes.
        };

        //!
        //! List of Cell entries.
        //!
        typedef std::list<Cell> CellList;

        // NetworkChangeNotifyDescriptor public members:
        CellList cells {};  //!< The list of cells and changes.

        //!
        //! Default constructor.
        //!
        NetworkChangeNotifyDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NetworkChangeNotifyDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
