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
//!
//!  @file
//!  Representation of a generic linkage_descriptor.
//!  Specialized classes exist, depending on the linkage type.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a generic linkage_descriptor.
    //! Specialized classes exist, depending on the linkage type.
    //! @see ETSI 300 468, 6.2.19.
    //!
    class TSDUCKDLL LinkageDescriptor : public AbstractDescriptor
    {
    public:
        // LinkageDescriptor public members:
        uint16_t    ts_id;         //!< Transport stream id.
        uint16_t    onetw_id;      //!< Original network id.
        uint16_t    service_id;    //!< Service id.
        uint8_t     linkage_type;  //!< Linkage type, LINKAGE_* constants, eg ts::LINKAGE_INFO.
        ByteBlock   private_data;  //!< Private data, depends on linkage type.

        //!
        //! Default constructor
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id.
        //! @param [in] service Service id.
        //! @param [in] ltype Linkage type.
        //!
        LinkageDescriptor(uint16_t ts = 0, uint16_t onetw = 0, uint16_t service = 0, uint8_t ltype = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LinkageDescriptor(const Descriptor& bin);

        // Inherited methods
        virtual void serialize(Descriptor&) const;
        virtual void deserialize(const Descriptor&);
        virtual XML::Element* toXML(XML& xml, XML::Document& doc) const;
        virtual void fromXML(XML& xml, const XML::Element* element);

        //!
        //! Static method to display a descriptor.
        //! @param [in,out] display Display engine.
        //! @param [in] did Descriptor id.
        //! @param [in] payload Address of the descriptor payload.
        //! @param [in] size Size in bytes of the descriptor payload.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //!
        static void DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds);
    };
}
