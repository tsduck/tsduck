//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Registry of plugin event handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsPluginEventHandlerInterface.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Registry of plugin event handlers.
    //! Used as base class for tsp and tsswitch implementations.
    //! @ingroup plugin
    //!
    class TSDUCKDLL PluginEventHandlerRegistry
    {
        TS_NOCOPY(PluginEventHandlerRegistry);
    public:
        //!
        //! Constructor.
        //!
        PluginEventHandlerRegistry() = default;

        //!
        //! Registration criteria for an event handler.
        //! A handler can be registered for any combination of:
        //! - Plugin name
        //! - Plugin index in the chain
        //! - Plugin type (input, packet processing, output).
        //! - Event code (32-bit plugin-specific value).
        //! - All plugins (when no criteria is specified).
        //!
        class TSDUCKDLL Criteria
        {
        public:
            std::optional<UString>    plugin_name {};   //!< When specified, the plugin must match that name.
            std::optional<size_t>     plugin_index {};  //!< When specified, the plugin must be at that index in the chain.
            std::optional<PluginType> plugin_type {};   //!< When specified, the plugin must be of this type.
            std::optional<uint32_t>   event_code {};    //!< When specified, the event must use that code.

            //!
            //! Default constructor.
            //! No criteria is set, meaning it matches all events.
            //!
            Criteria() = default;

            //!
            //! Constructor with an event code.
            //! It matches all events with that code from any plugin.
            //! @param [in] code Event code.
            //!
            Criteria(uint32_t code) : event_code(code) {}

            //!
            //! Constructor with a plugin type.
            //! It matches all events from any plugin of that type.
            //! @param [in] type Plugin type.
            //!
            Criteria(PluginType type) : plugin_type(type) {}

            //!
            //! Constructor with a plugin name.
            //! It matches all events from any plugin of that name.
            //! @param [in] name Plugin name.
            //!
            Criteria(const UString& name) : plugin_name(name) {}

            //!
            //! A common empty criteria, meaning "any event".
            //!
            static const Criteria Any;
        };

        //!
        //! Register an event handler.
        //! Note: calling this function while executing a plugin event hander does nothing.
        //! @param [in] handler The event handler to register.
        //! @param [in] criteria The criteria for which the handler is to be called.
        //!
        void registerEventHandler(PluginEventHandlerInterface* handler, const Criteria& criteria = Criteria::Any);

        //!
        //! Unregister all occurences of an event handler.
        //! Note: calling this function while executing a plugin event hander does nothing.
        //! This is typically used in the destructor of an event handler.
        //! @param [in] handler The event handler to unregister. When null, all events are unregistered.
        //!
        void unregisterEventHandler(PluginEventHandlerInterface* handler = nullptr);

        //!
        //! Invoke all event handlers for a given event.
        //! @param [in] context Event context.
        //!
        void callEventHandlers(const PluginEventContext& context) const;

    private:
        // It is difficult to find an efficient method to lookup registered handlers,
        // due to the combinations of criteria. We store them in a sequential list.
        // Since we do not expect many handlers and many events, that should be ok.
        using HandlerEntry = std::pair<PluginEventHandlerInterface*, Criteria>;
        using HandlerEntryList = std::list<HandlerEntry>;

        // Accessing the list, including executing an event handler is done under a mutex.
        mutable std::recursive_mutex _mutex {};
        mutable bool _calling_handlers = false;
        HandlerEntryList _handlers {};
    };
}
