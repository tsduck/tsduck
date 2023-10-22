//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * This abstract class is the base of all TSDuck classes which are backed by a C++ object.
 * @ingroup java
 *
 * There is an inherent problem when garbage-collected languages such as Java are interfaced
 * with languages using explicit memory management. When a Java class encapsulates a native
 * C++ object of the corresponding C++ class, when should the C++ object be deleted?
 * This is a problem which has been discussed many times on the Internet and the answer
 * is disappointing: there is no good solution.
 *
 *  1. A naive approach would be to override java.lang.Object.finalize() and perform the
 *     C++ deallocation here. It is well known that finalize() creates more issues than it
 *     solves. Specifically, we cannot guarantee the order of finalization of objects, which
 *     could lead to crashes when C++ objects reference each other is a predetermined order.
 *
 *  2. Never delete C++ objects and let them accumulate. This can be acceptable if a
 *     guaranteed maximum number of C++ objects are allocated during the life of the
 *     application and the corresponding memory usage is acceptable.
 *
 *  3. Expose a public method in all Java classes which deletes, frees, deallocates, you name it,
 *     the encapsulated C++ object. It is then the responsibility of the application to call
 *     this method on time. This is counter-intuitive to both Java and C++ programmers but
 *     this is the price to pay when you want to use them together.
 *
 * In the Java TSDuck bindings, all classes which encapsulate a C++ object implement the
 * interface NativeObject which provides the delete() method to explicitly delete the C++
 * object. In practice, users have the choice between solutions 2 or 3.
 */
public abstract class NativeObject {

    /*
     * Load TSDuck native library on startup.
     */
    static {
        NativeLibrary.loadLibrary();
    }

    /**
     * The address of the underlying C++ object.
     * It is normally accessed by native methods only.
     */
    protected long nativeObject = 0;

    /**
     * Explicitly free the underlying C++ object.
     *
     * After this call, the object becomes unusable.
     * Most usages are unpredictable but most likely will do nothing.
     */
    public abstract void delete();
}
