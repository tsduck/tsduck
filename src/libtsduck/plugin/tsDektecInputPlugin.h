//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Declare the ts::DektecInputPlugin class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"

namespace ts {
    //!
    //! Dektec input plugin for @c tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL DektecInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(DektecInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        DektecInputPlugin(TSP* tsp);

        //!
        //! Destructor.
        //!
        virtual ~DektecInputPlugin();

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual size_t stackUsage() const override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;

        //! @cond nodoxygen
        // A dummy storage value to force inclusion of this module when using the static library.
        static const int REFERENCE;
        //! @endcond

    private:
        class Guts;
        Guts* _guts;

        // Configure the LNB. Return true on success.
        bool configureLNB();
    };
}
