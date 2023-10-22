//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * A wrapper class for C++ NullReport.
 * @ingroup java
 *
 * Since the corresponding C++ class is a singleton, there is no delete() method.
 */
public final class NullReport extends Report {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject();

    /**
     * Constructor
     */
    public NullReport() {
        initNativeObject();
    }

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public void delete() {
        // This is a C++ singleton, not to be deleted.
    }
}
