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
//!  Representation of a Network Information Table (NIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTransportListTable.h"

namespace ts {
    //!
    //! Representation of a Network Information Table (NIT)
    //!
    class TSDUCKDLL NIT : public AbstractTransportListTable
    {
    public:
        // NIT public members:
        uint16_t& network_id;  //!< Network identifier.

        //!
        //! Default constructor.
        //! @param [in] is_actual True for NIT Actual Network, false for NIT Other Network.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //! @param [in] id Network identifier.
        //!
        NIT(bool is_actual = true, uint8_t vers = 0, bool cur = true, uint16_t id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //!
        NIT(const BinaryTable& table);

        //!
        //! Check if this is an "actual" NIT.
        //! @return True for NIT Actual Network, false for NIT Other Network.
        //!
        bool isActual() const
        {
            return _table_id == TID_NIT_ACT;
        }

        //!
        //! Set if this is an "actual" NIT.
        //! @param [in] is_actual True for NIT Actual Network, false for NIT Other Network.
        //!
        void setActual(bool is_actual)
        {
            _table_id = uint8_t(is_actual ? TID_NIT_ACT : TID_NIT_OTH);
        }

        // Inherited methods
        virtual XML::Element* toXML(XML& xml, XML::Document& doc) const;
        virtual void fromXML(XML& xml, const XML::Element* element);

        //!
        //! A static method to display a section.
        //! @param [in,out] display Display engine.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        static void DisplaySection(TablesDisplay& display, const Section& section, int indent);
    };
}
