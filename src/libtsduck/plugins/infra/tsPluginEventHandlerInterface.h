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
//!  Abstract interface to receive events from a plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventContext.h"

namespace ts {
    //!
    //! Abstract interface to receive events from a plugin.
    //! @ingroup plugin
    //!
    //! This abstract interface must be implemented by classes which need to
    //! receive events from plugins.
    //!
    //! Warnings:
    //! - Event handlers are invoked in the context of a plugin handler thread.
    //! - A global mutext is used for registration and execution of event handlers.
    //! - Event handler execution shall be as short as possible because:
    //!   - It blocks the execution of the plugin thread (synchronous call).
    //!   - It blocks execution of all other event handlers from all plugins (global mutex).
    //!
    class TSDUCKDLL PluginEventHandlerInterface
    {
    public:
        //!
        //! This handler is invoked when a plugin signals an event for which this object is registered.
        //! @param [in] context Context containing all information about the event.
        //!
        virtual void handlePluginEvent(const PluginEventContext& context) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~PluginEventHandlerInterface();
    };
}
