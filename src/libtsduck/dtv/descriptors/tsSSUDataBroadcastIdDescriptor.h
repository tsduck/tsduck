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
//!  Representation of a data_broadcast_id_descriptor for SSU.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsDataBroadcastIdDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of a data_broadcast_id_descriptor for system software update.
    //! The data_broadcast_id is 0x000A.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SSUDataBroadcastIdDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! OUI entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint32_t          oui;             //!< OUI, 24 bits.
            uint8_t           update_type;     //!< Update type, 4 bits.
            Variable<uint8_t> update_version;  //!< Update version, 5 bits.
            ByteBlock         selector;        //!< Selector bytes.

            //!
            //! Constructor.
            //! @param [in] oui_ OUI, 24 bits.
            //! @param [in] upd_ Update type, 4 bits.
            //!
            Entry(uint32_t oui_ = 0, uint8_t upd_ = 0);
        };

        //!
        //! List of OUI entries.
        //!
        typedef std::list<Entry> EntryList;

        // SSUDataBroadcastIdDescriptor public members:
        EntryList entries;       //!< The list of OUI entries.
        ByteBlock private_data;  //!< Private data.

        //!
        //! Default constructor.
        //!
        SSUDataBroadcastIdDescriptor();

        //!
        //! Constructor with one OUI.
        //! @param [in] oui OUI, 24 bits.
        //! @param [in] update_type Update type, 4 bits.
        //!
        SSUDataBroadcastIdDescriptor(uint32_t oui, uint8_t update_type);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SSUDataBroadcastIdDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Constructor from a data_broadcast_id_descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc A data_broadcast_id_descriptor to convert.
        //! The data_broadcast_id must be 0x000A.
        //!
        SSUDataBroadcastIdDescriptor(DuckContext& duck, const DataBroadcastIdDescriptor& desc);

        //!
        //! Convert to a data_broadcast_id_descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] desc A data_broadcast_id_descriptor to convert.
        //!
        void toDataBroadcastIdDescriptor(DuckContext& duck, DataBroadcastIdDescriptor& desc) const;

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
