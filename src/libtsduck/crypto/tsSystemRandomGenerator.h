//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        ::HCRYPTPROV _prov = 0;
#else
        int _fd = -1;
#endif
    };
}
