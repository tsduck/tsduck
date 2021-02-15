//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  General-purpose plugin event data referencing constant binary data.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"

namespace ts {
    //!
    //! General-purpose plugin event data referencing constant binary data.
    //! @ingroup plugin
    //!
    //! This subclass of Object can be used as "plugin data" when a plugin triggers
    //! an event and wants to pass to the application a read-only binary area.
    //!
    //! The plugin event handlers in the application are synchronously invoked in
    //! the context of the thread plugin. The referenced binary data can be local
    //! data inside the plugin. The event handler may read it but not write it and
    //! not saved a reference to it.
    //!
    class TSDUCKDLL PluginEventData : public Object
    {
        // Prevent copy to allow safe storage of references.
        TS_NOBUILD_NOCOPY(PluginEventData);
    public:
        //!
        //! Constructor.
        //! @param [in] data Address of the plugin data to pass to applications. It can be a null pointer.
        //! @param [in] size Size in bytes of the plugin data.
        //!
        PluginEventData(const uint8_t* data, size_t size);

        //!
        //! Destructor.
        //!
        ~PluginEventData();

        //!
        //! Get the address of the plugin data.
        //! @return The address of the plugin data.
        //!
        const uint8_t* data() const { return _data; }

        //!
        //! Get the size in bytes of the plugin data.
        //! @return The size in bytes of the plugin data.
        //!
        size_t size() const { return _size; }

    private:
        const uint8_t* const _data;
        const size_t _size;
    };
}
