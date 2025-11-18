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

namespace ts {
    //!
    //! Representation of a file extracted from a FLUTE stream.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteFile
    {
    public:
        //@@ TO BE COMPLETED.

        //!
        //! Default constructor.
        //!
        FluteFile() = default;

        //!
        //! Clear the content of this object.
        //!
        void clear();

        //!
        //! Get the size of the file.
        //! @return The size of the file in bytes.
        //!
        size_t size() const { return _data.size(); }

        //!
        //! Access the content of the file (modifiable version).
        //! @return A reference to the content of the file.
        //!
        ByteBlock& content() { return _data; }

        //!
        //! Access the content of the file (constant version).
        //! @return A constant reference to the content of the file.
        //!
        const ByteBlock& content() const { return _data; }

    private:
        ByteBlock _data {};  // File content.
    };
}
