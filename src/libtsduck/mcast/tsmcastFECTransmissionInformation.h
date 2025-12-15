//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the FEC Transmission Information in FLUTE headers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"

namespace ts::mcast {
    //!
    //! Representation of the FEC Transmission Information in FLUTE headers.
    //! @see IETF RFC 3926, section 5.1.1
    //! @ingroup libtsduck mpeg
    //!
    //! If a file is not content-encoded, the transfer length is the same as the file length.
    //!
    class TSDUCKDLL FECTransmissionInformation : public StringifyInterface
    {
    public:
        uint8_t  fec_encoding_id = 0;          //!< FEC Encoding ID which was used to parse the structure (not part of the structore).
        uint64_t transfer_length = 0;          //!< The length of the transport object that carries the file in bytes.
        uint16_t fec_instance_id = 0;          //!< FEC Instance ID (FEC Encoding ID 128-255).
        uint16_t encoding_symbol_length = 0;   //!< Length of Encoding Symbol in bytes (FEC Encoding ID 0, 128, 129, 130).
        uint32_t max_source_block_length = 0;  //!< Max number of source symbols per source block (FEC Encoding ID 0, 128, 129, 130).
        uint16_t max_encoding_symbols = 0;     //!< Max number of encoding symbols (FEC Encoding ID 129).

        //!
        //! Default constructor.
        //!
        FECTransmissionInformation() = default;

        //!
        //! Clear the content of a structure.
        //!
        void clear();

        //!
        //! Deserialize the structure from a binary area.
        //! @param [in] fec_encoding_id FEC Encoding ID, required to interpret the format of the binary area.
        //! @param [in] addr Address of binary area.
        //! @param [in] size Size of binary area.
        //! @return True on success, false on error. Same as @a valid field.
        //!
        bool deserialize(uint8_t fec_encoding_id, const uint8_t* addr, size_t size);

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };
}
