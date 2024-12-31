//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(PluginEventHandlerInterface);
    public:
        //!
        //! This handler is invoked when a plugin signals an event for which this object is registered.
        //! @param [in] context Context containing all information about the event.
        //!
        virtual void handlePluginEvent(const PluginEventContext& context) = 0;
    };
}
