//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
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
