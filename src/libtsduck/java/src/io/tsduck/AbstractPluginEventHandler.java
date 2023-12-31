//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2024, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * An abstract class which can be derived by applications to get plugin events.
 * @ingroup java
 */
public abstract class AbstractPluginEventHandler extends NativeObject {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject(String handlerMethodName);

    /**
     * Constructor (for subclasses).
     */
    protected AbstractPluginEventHandler() {
        initNativeObject("handlePluginEvent");
    }

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();

    /**
     * This handler is invoked when a plugin signals an event for which this object is registered.
     * The application should override it to collect the event.
     *
     * The associated input event data is passed in @a data. If @a context.readOnlyData() is
     * false, it is possible to update the data. This is typically the case with the @e memory
     * input plugin which signals events with empty input data and expects TS packets as returned
     * data. The updated data, if any, should be set by the handler using @a context.setOutputData().
     * The size of the returned data shall not exceed @a context.maxDataSize(). Otherwise,
     * it will be ignored.
     *
     * @param context An instance of PluginEventContext containing the details of the event.
     * @param data A byte array containing the data of the event. This is a read-only
     * sequence of bytes. There is no way to return data from Java to the plugin.
     * @return True in case of success, false to set the error indicator of the event.
     */
    abstract public boolean handlePluginEvent(PluginEventContext context, byte[] data);
}
