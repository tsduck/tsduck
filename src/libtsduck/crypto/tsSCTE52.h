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
//!  ANSI/SCTE 52 DES-based TS packet encryption.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVS042.h"
#include "tsDES.h"

namespace ts {
    //!
    //! ANSI/SCTE 52 2003 DES-based TS packet encryption.
    //! @ingroup crypto
    //!
    //! DVS 042 has been renamed as "ANSI/SCTE 52 2003". The next iteration of
    //! this standard is "ANSI/SCTE 52 2008". The only difference between the
    //! two versions is the handling of messages shorter than the block size.
    //! In the 2003 (DVS 042) version, the same IV (called "whitener" in the
    //! standard) is used for long and short messages. In the 2008 version, a
    //! different "whitener2" must be used for messages shorter than the block size.
    //!
    class TSDUCKDLL SCTE52_2003 : public DVS042<DES>
    {
        TS_NOCOPY(SCTE52_2003);
    public:
        //!
        //! Constructor.
        //!
        SCTE52_2003() = default;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    private:
        // The IV are identical, there is no specific IV for short blocks.
        virtual bool setShortIV(const void* iv_, size_t iv_length) override;
    };

    //!
    //! ANSI/SCTE 52 2008 DES-based TS packet encryption.
    //! @ingroup crypto
    //!
    //! DVS 042 has been renamed as "ANSI/SCTE 52 2003". The next iteration of
    //! this standard is "ANSI/SCTE 52 2008". The only difference between the
    //! two versions is the handling of messages shorter than the block size.
    //! In the 2003 (DVS 042) version, the same IV (called "whitener" in the
    //! standard) is used for long and short messages. In the 2008 version, a
    //! different "whitener2" must be used for messages shorter than the block size.
    //!
    class TSDUCKDLL SCTE52_2008 : public DVS042<DES>
    {
        TS_NOCOPY(SCTE52_2008);
    public:
        //!
        //! Constructor.
        //!
        SCTE52_2008() = default;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;
    };
}
