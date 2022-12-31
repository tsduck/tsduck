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
//!  ATIS-0800006 AES-based TS packet encryption (ATIS-IDSA).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVS042.h"
#include "tsAES.h"

namespace ts {
    //!
    //! ATIS-0800006 AES-based TS packet encryption (ATIS-IDSA).
    //! @ingroup crypto
    //!
    class TSDUCKDLL IDSA : public DVS042<AES>
    {
        TS_NOCOPY(IDSA);
    public:
        //!
        //! ATIS-IDSA control words size in bytes (AES-128 key size).
        //!
        static const size_t KEY_SIZE = 16;

        //!
        //! Constructor.
        //!
        IDSA();

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    private:
        virtual bool setIV(const void* iv_, size_t iv_length) override;
        virtual bool setShortIV(const void* iv_, size_t iv_length) override;
    };
}
