//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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
 * A wrapper class for C++ SectionFile.
 * @ingroup java
 */
public class SectionFile implements NativeObject {

    // Load native library on startup.
    static {
        NativeLibrary.loadLibrary();
    }

    // The address of the underlying C++ object.
    private long nativeObject = 0;

    // The address of a self-allocated C++ Duck context object, if null was provided in the constructor.
    // Will be deallocated with the C++ SectionFile object.
    private long nativeDuckContext = 0;

    // Set the address of the C++ object.
    private native void initNativeObject(DuckContext duck);

    /**
     * Constructor
     * @param duck The TSDuck execution context object to use.
     */
    public SectionFile(DuckContext duck) {
        initNativeObject(duck);
    }

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();

    /**
     * Clear the content of the SectionFile, erase all sections.
     */
    public native void clear();

    /**
     * Get the size in bytes of all sections.
     * This would be the size of the corresponding binary file.
     * @return The size in bytes of all sections.
     */
    public native int binarySize();

    /**
     * Get the total number of sections in the file.
     * @return The total number of sections in the file.
     */
    public native int sectionsCount();

    /**
     * Get the total number of full tables in the file.
     * Orphan sections are not included.
     * @return The total number of full tables in the file.
     */
    public native int tablesCount();

    /**
     * Load a binary section file from a memory buffer.
     * The loaded sections are added to the content of this object.
     * @param data A byte array containing the binary data to load.
     * @return True on success, False if some sections were incorrect or truncated.
     */
    public native boolean fromBinary(byte[] data);

    /**
     * Get the binary content of a section file.
     * @return A byte array containing the binary sections.
     */
    public native byte[] toBinary();

    /**
     * Load a binary section file.
     * The loaded sections are added to the content of this object.
     * @param file Binary file name. If the file name is empty or "-", the standard input is used.
     * @return True on success, False on error.
     */
    public native boolean loadBinary(String file);

    /**
     * Save a binary section file.
     * @param [in] file Binary file name. If the file name is empty or "-", the standard output is used.
     * @return True on success, False on error.
     */
    public native boolean saveBinary(String file);

    /**
     * Load an XML file.
     * The loaded tables are added to the content of this object.
     * @param file XML file name. If the file name is empty or "-", the standard input is used.
     * If the file name starts with "<?xml", this is considered as "inline XML content".
     * @return True on success, False on error.
     */
    public native boolean loadXML(String file);

    /**
     * Save an XML file.
     * @param [in] file XML file name. If the file name is empty or "-", the standard output is used.
     * @return True on success, False on error.
     */
    public native boolean saveXML(String file);

    /**
     * Save a JSON file after automated XML-to-JSON conversion.
     * @param [in] file JSON file name. If the file name is empty or "-", the standard output is used.
     * @return True on success, False on error.
     */
    public native boolean saveJSON(String file);

    /**
     * Serialize as XML text.
     * @return Complete XML document text, empty on error.
     */
    public native String toXML();

    /**
     * Serialize as JSON text.
     * @return Complete JSON document text, empty on error.
     */
    public native String toJSON();
}
