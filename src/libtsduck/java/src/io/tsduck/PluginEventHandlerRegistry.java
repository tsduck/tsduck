//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * A wrapper class for C++ PluginEventHandlerRegistry.
 * @ingroup java
 */
public abstract class PluginEventHandlerRegistry extends NativeObject {

    /**
     * Register an event handler by event code.
     * @param handler An instance of AbstractPluginEventHandler.
     * @param eventCode The code of the events to handle.
     */
    public native void registerEventHandler(AbstractPluginEventHandler handler, int eventCode);

    /**
     * Register an event handler for all events from the input plugin.
     * @param handler An instance of AbstractPluginEventHandler.
     */
    public native void registerInputEventHandler(AbstractPluginEventHandler handler);

    /**
     * Register an event handler for all events from the output plugin.
     * @param handler An instance of AbstractPluginEventHandler.
     */
    public native void registerOutputEventHandler(AbstractPluginEventHandler handler);
}
