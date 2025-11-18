//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a file extracted from a FLUTE stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! Representation of a file extracted from a FLUTE stream.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteFile
    {
    public:
        //!
        //! Constructor.
        //! @param [in] source Source IP address.
        //! @param [in] destination Destination socket address.
        //! @param [in] tsi Transport Session Identifier.
        //! @param [in] toi Transport Object Identifier.
        //! @param [in] name File name or URN.
        //! @param [in] content File content.
        //!
        FluteFile(const IPAddress&       source,
                  const IPSocketAddress& destination,
                  uint64_t               tsi,
                  uint64_t               toi,
                  const UString&         name,
                  const ByteBlockPtr&    content);

        //!
        //! Get the source IP address which sent the file.
        //! @return A constant reference to the IP address in the object.
        //!
        const IPAddress& source() const { return _source; }

        //!
        //! Get the destination IP address and UDP port to which the file was sent.
        //! @return A constant reference to the IP socket address in the object.
        //!
        const IPSocketAddress& destination() const { return _destination; }

        //!
        //! Get the Transport Session Identifier (TSI) of the file.
        //! @return The Transport Session Identifier
        //!
        uint64_t tsi() const { return _tsi; }

        //!
        //! Get the Transport Object Identifier (TOI) of the file.
        //! @return The Transport Object Identifier
        //!
        uint64_t toi() const { return _toi; }

        //!
        //! Get the name or URN of the file.
        //! @return A constant reference to the name in the object.
        //!
        const UString& name() const { return _name; }

        //!
        //! Get the size of the file.
        //! @return The size of the file in bytes.
        //!
        size_t size() const { return _content->size(); }

        //!
        //! Access the content of the file (modifiable version).
        //! @return A reference to the content of the file.
        //!
        ByteBlock& content() { return *_content; }

        //!
        //! Access the content of the file (constant version).
        //! @return A constant reference to the content of the file.
        //!
        const ByteBlock& content() const { return *_content; }

    private:
        IPAddress       _source;
        IPSocketAddress _destination;
        uint64_t        _tsi;
        uint64_t        _toi;
        UString         _name;
        ByteBlockPtr    _content;
    };
}
