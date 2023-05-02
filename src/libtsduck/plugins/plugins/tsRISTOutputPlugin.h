//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
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
//!  Reliable Internet Stream Transport (RIST) output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDatagramOutputPlugin.h"

namespace ts {
    //!
    //! Reliable Internet Stream Transport (RIST) output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL RISTOutputPlugin: public AbstractDatagramOutputPlugin
    {
        TS_NOBUILD_NOCOPY(RISTOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        RISTOutputPlugin(TSP* tsp);

        //!
        //! Destructor.
        //!
        virtual ~RISTOutputPlugin() override;

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool isRealTime() override;
        virtual bool start() override;
        virtual bool stop() override;

    protected:
        // Implementation of AbstractDatagramOutputPlugin.
        virtual bool sendDatagram(const void* address, size_t size) override;

    private:
        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts;
    };
}


