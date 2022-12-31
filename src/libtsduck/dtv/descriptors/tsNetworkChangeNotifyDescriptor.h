//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Representation of a network_change_notify_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"
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
            Change();                                  //!< Default constructor.
            uint8_t            network_change_id;      //!< Network change id.
            uint8_t            network_change_version; //!< Network change version.
            Time               start_time_of_change;   //!< Start time of change.
            Second             change_duration;        //!< Change duration in seconds (must be less than 12 hours)?
            uint8_t            receiver_category;      //!< 3 bits, 0 for all, 1 for T2/S2/C2.
            uint8_t            change_type;            //!< 4 bits, type of change.
            uint8_t            message_id;             //!< Message id.
            Variable<uint16_t> invariant_ts_tsid;      //!< Optional invariant TS id.
            Variable<uint16_t> invariant_ts_onid;      //!< Original network id of optional invariant TS.
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
            Cell();               //!< Default constructor.
            uint16_t   cell_id;   //!< Cell id.
            ChangeList changes;   //!< List of changes.
        };

        //!
        //! List of Cell entries.
        //!
        typedef std::list<Cell> CellList;

        // NetworkChangeNotifyDescriptor public members:
        CellList cells;  //!< The list of cells and changes.

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
