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
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] net_id Network identifier.
        //!
        NIT(bool is_actual = true, uint8_t version = 0, bool is_current = true, uint16_t net_id = 0) :
            AbstractTransportListTable(uint8_t(is_actual ? TID_NIT_ACT : TID_NIT_OTH), net_id, version, is_current),
            network_id(_tid_ext)
        {
        }

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //!
        NIT(const BinaryTable& table) :
            AbstractTransportListTable(TID_NIT_ACT, table),  // TID updated by Deserialize
            network_id(_tid_ext)
        {
        }            

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
    };
}
