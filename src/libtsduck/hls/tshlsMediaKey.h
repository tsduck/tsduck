//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of an encryption key in an HLS media segment.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshlsMediaElement.h"
#include "tsByteBlock.h"

namespace ts::hls {
    //!
    //! Description of an encryption key in an HLS media segment.
    //! @ingroup libtsduck hls
    //!
    class TSDUCKDLL MediaKey: public MediaElement
    {
        TS_RULE_OF_FIVE(MediaKey, override);
    public:
        //!
        //! Constructor.
        //!
        MediaKey() = default;

        UString   method {};  //!< Encryption key method (empty or "NONE" means unencrypted).
        UString   format {};  //!< Encryption key format (empty or "identity" means raw binary).
        ByteBlock iv {};      //!< Initialization vector for encryption.

        //!
        //! Check if the media segment is encrypted.
        //! @return True if the media segment is encrypted, false otherwise.
        //!
        bool isEncrypted() const;

        //!
        //! Check if the encryption key is in the default raw binary format.
        //! @return True if the encryption key is in the default raw binary format, false otherwise.
        //!
        bool isRawEncryptionKey() const;
    };
}
