//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the FEC Payload ID in FLUTE headers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"

namespace ts::mcast {
    //!
    //! Representation of the FEC Payload ID in FLUTE headers.
    //! @see IETF RFC 5775, section 2
    //! @ingroup libtsduck mpeg
    //!
    //! If a file is not content-encoded, the transfer length is the same as the file length.
    //!
    class TSDUCKDLL FECPayloadId : public StringifyInterface
    {
    public:
        bool    valid = false;            //!< The information was successfully parsed.
        uint8_t fec_encoding_id = 0;      //!< FEC Encoding ID which was used to parse the structure (not part of the structore).
        size_t  source_block_number = 0;  //!< SBN, Source Block Number (FEC Encoding ID 0 and 130, RFC 3695, section 2.1).
        size_t  encoding_symbol_id = 0;   //!< Encoding Symbol ID (FEC Encoding ID 0 and 130, RFC 3695, section 2.1).

        //!
        //! Default constructor.
        //!
        FECPayloadId() = default;

        //!
        //! Clear the content of a structure.
        //!
        void clear();

        //!
        //! Deserialize the structure from a binary area.
        //! @param [in] fec_encoding_id FEC Encoding ID, required to interpret the format of the binary area.
        //! @param [in,out] addr Address of binary area. Updated to point after the structure.
        //! @param [in,out] size Size of binary area. Updated with remaining size after deserializing the structure.
        //! @return True on success, false on error. Same as @a valid field.
        //!
        bool deserialize(uint8_t fec_encoding_id, const uint8_t*& addr, size_t& size);

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };
}
