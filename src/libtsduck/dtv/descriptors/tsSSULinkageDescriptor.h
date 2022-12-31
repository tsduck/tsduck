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
//!  Representation of a linkage_descriptor for SSU.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsLinkageDescriptor.h"

namespace ts {
    //!
    //! Representation of a linkage_descriptor for system software update.
    //! SSU uses linkage type 0x09.
    //! @see ETSI EN 300 468, 6.2.19.
    //! @see ETSI TS 102 006, 6.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SSULinkageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! OUI entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint32_t  oui;       //!< OUI, 24 bits.
            ByteBlock selector;  //!< Selector bytes.

            //!
            //! Constructor.
            //! @param [in] oui_ OUI, 24 bits.
            //!
            Entry(uint32_t oui_ = 0);
        };

        //!
        //! List of OUI entries.
        //!
        typedef std::list<Entry> EntryList;

        // SSULinkageDescriptor public members:
        uint16_t  ts_id;         //!< Transport stream id.
        uint16_t  onetw_id;      //!< Original network id.
        uint16_t  service_id;    //!< Service id.
        EntryList entries;       //!< The list of OUI entries.
        ByteBlock private_data;  //!< Private data.

        //!
        //! Default constructor.
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id
        //! @param [in] service Service id.
        //!
        SSULinkageDescriptor(uint16_t ts = 0, uint16_t onetw = 0, uint16_t service = 0);

        //!
        //! Constructor with one OUI.
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id
        //! @param [in] service Service id.
        //! @param [in] oui OUI, 24 bits.
        //!
        SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint32_t oui);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SSULinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Constructor from a linkage_descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc A linkage_descriptor to convert.
        //! The data_broadcast_id must be 0x000A.
        //!
        SSULinkageDescriptor(DuckContext& duck, const LinkageDescriptor& desc);

        //!
        //! Convert to a linkage_descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] desc A linkage_descriptor to convert.
        //!
        void toLinkageDescriptor(DuckContext& duck, LinkageDescriptor& desc) const;

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
