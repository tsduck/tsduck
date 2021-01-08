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

import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * An internal class to interface the TSDuck C++ shared library.
 * @ingroup java
 */
class NativeLibrary {

    /**
     * Name of the native library.
     */
    private static final String libName = "tsduck";

    /**
     * Native method inside the TSDuck library which initializes what needs to be initialized.
     */
    private static native void initialize();

    /**
     * Boolean to be set once the native library is loaded to avoid loading it twice.
     */
    private static boolean loaded = false;

    /**
     * Get the operating system name.
     * @return The operating system name, lowercase, without space.
     */
    private static String osName() {
        return System.getProperty("os.name").toLowerCase().replaceAll(" ", "");
    }

    /**
     * Search a library in a search path.
     * @param search Search path.
     * @param lib Library name ("link name").
     * @return Library file path or null if not found.
     */
    private static String searchLibraryInPath(String search, String lib) {
        if (search != null) {
            for (String dir : search.split(":")) {
                if (!dir.isEmpty()) {
                    Path path = FileSystems.getDefault().getPath(dir, "lib" + lib + ".so");
                    if (Files.exists(path)) {
                        return path.toString();
                    }
                    path = FileSystems.getDefault().getPath(dir, "lib" + lib + ".dylib");
                    if (Files.exists(path)) {
                        return path.toString();
                    }
                }
            }
        }
        return null; // not found
    }

    /**
     * This static method loads the native TSDuck library.
     * It is automatically invoked by low-level classes which require the native library.
     */
    public static synchronized void loadLibrary() {
        if (!loaded) {
            String path = null;
            if (osName().startsWith("mac")) {
                // On macOS with "system integrity protection", LD_LIBRARY_PATH is not
                // passed to Java processes and the java.libray.path does not contain
                // the 'lib' directory. We must do the search explicitly.
                path = searchLibraryInPath(System.getenv("TSPLUGINS_PATH"), libName);
                if (path == null) {
                    path = searchLibraryInPath("/usr/local/lib:/usr/lib", libName);
                }
            }
            // Load the native library by explicit file name or using system search rules.
            if (path != null) {
                System.load(path.toString());
            }
            else {
                System.loadLibrary(libName);
            }
            initialize();
            loaded = true;
        }
    }
}
