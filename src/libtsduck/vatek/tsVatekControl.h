//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Vision Advance Technology Inc. (VATek)
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
//!  A class implementing the @c tsvatek control utility.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsDuckContext.h"

namespace ts {
    //!
    //! A class implementing the tsvatek control utility.
    //! This is defined as a separate class the interface of which does not depend on the Vatek core library.
    //! @ingroup hardware
    //!
    class TSDUCKDLL VatekControl: private Args
    {
        TS_NOBUILD_NOCOPY(VatekControl);
    public:
        //!
        //! Constructor.
        //! @param [in] argc Command line argument count.
        //! @param [in] argv Command line arguments.
        //!
        VatekControl(int argc, char *argv[]);

        //!
        //! Destructor.
        //!
        virtual ~VatekControl() override;

        //!
        //! Execute the command.
        //! @return Either EXIT_SUCCESS or EXIT_FAILURE.
        //!
        int execute();

    private:
        DuckContext _duck;
        int32_t     _dev_index;  // Vatek device index, -1 for all devices
    };
}
