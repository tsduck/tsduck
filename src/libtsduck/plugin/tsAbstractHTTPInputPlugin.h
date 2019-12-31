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
//!  Abstract base class for HTTP-based input plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPushInputPlugin.h"
#include "tsWebRequestHandlerInterface.h"
#include "tsTSFile.h"

namespace ts {
    //!
    //! Abstract base class for HTTP-based input plugins.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractHTTPInputPlugin: public PushInputPlugin, protected WebRequestHandlerInterface
    {
        TS_NOBUILD_NOCOPY(AbstractHTTPInputPlugin);
    public:
        // Implementation of Plugin interface.
        // If overridden by descrambler subclass, superclass must be explicitly invoked.
        virtual bool start() override;

        // Inherited from PushInputPluginÒ.
        virtual bool pushPackets(const TSPacket* buffer, size_t count) override;

        //!
        //! Set a directory name where all loaded files are automatically saved.
        //! @param [in] dir A directory name.
        //!
        void setAutoSaveDirectory(const UString dir) { _autoSaveDir = dir; }

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Object to communicate with the Transport Stream Processor main executable.
        //! @param [in] description A short one-line description, eg. "Descrambler for 'xyz' CAS".
        //! @param [in] syntax A short one-line syntax summary, default: u"[options] [service]".
        //!
        AbstractHTTPInputPlugin(TSP* tsp, const UString& description, const UString& syntax);

        // Implementation of WebRequestHandlerInterface
        virtual bool handleWebStart(const WebRequest& request, size_t size) override;
        virtual bool handleWebData(const WebRequest& request, const void* data, size_t size) override;
        virtual bool handleWebStop(const WebRequest& request) override;

    private:
        TSPacket     _partial;       // Buffer for incomplete packets.
        size_t       _partial_size;  // Number of bytes in partial.
        UString      _autoSaveDir;   // If not empty, automatically save loaded files to this directory.
        TSFile       _outSave;       // TS file where to store the loaded file.
    };
}
