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
//!  System-provided PRNG (pseudo-random numbers generator).
//!  Usually not the best PRNG on earth, but fine for most usages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsRandomGenerator.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <Wincrypt.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! System-provided PRNG (pseudo-random numbers generator).
    //! Usually not the best PRNG on earth, but fine for most usages.
    //! @ingroup crypto
    //!
    class TSDUCKDLL SystemRandomGenerator: public RandomGenerator
    {
        TS_NOCOPY(SystemRandomGenerator);
    public:
        // Implementation of RandomGenerator interface:
        virtual UString name() const override;
        virtual bool seed(const void*, size_t) override;
        virtual bool ready() const override;
        virtual bool read(void*, size_t) override;

        //!
        //! Constructor.
        //!
        SystemRandomGenerator();

        //!
        //! Virtual destructor
        //!
        virtual ~SystemRandomGenerator() override;

    private:
#if defined(TS_WINDOWS)
        ::HCRYPTPROV _prov;
#else
        int _fd;
#endif
    };
}
