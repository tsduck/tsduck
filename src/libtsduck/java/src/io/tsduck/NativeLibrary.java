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

import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * An internal class to interface the TSDuck C++ shared library.
 * @ingroup java
 */
class NativeLibrary {

    /*
     * Operating system features.
     */
    private static final String pathSeparator = System.getProperty("path.separator");
    private static final String osName = System.getProperty("os.name").toLowerCase().replaceAll(" ", "");
    private static final boolean isWindows = osName.startsWith("win") || osName.startsWith("nt");
    private static final boolean isMac = osName.startsWith("mac");

    /*
     * TSDuck native library.
     */
    private static final String libName = "tsduck";
    private static boolean loaded = false;

    /**
     * Native method inside the TSDuck library which initializes what needs to be initialized.
     */
    private static native void initialize();

    /**
     * Search a library in a search path.
     * @param search Search path.
     * @param lib Library name ("link name").
     * @return Library file path or null if not found.
     */
    private static String searchLibraryInPath(String search, String lib) {
        if (search != null && lib != null) {
            // Build OS-dependent library file name.
            String fileName;
            if (isWindows) {
                fileName = lib + ".dll";
            }
            else if (isMac) {
                fileName = "lib" + lib + ".dylib";
            }
            else {
                fileName = "lib" + lib + ".so";
            }
            // Search in all directories in the search path.
            for (String dir : search.split(pathSeparator)) {
                if (!dir.isEmpty()) {
                    Path path = FileSystems.getDefault().getPath(dir, fileName);
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
            // Try in same directory as plugins (typically in a development environment).
            path = searchLibraryInPath(System.getenv("TSPLUGINS_PATH"), libName);
            // Try various typical Unix directories
            if (path == null && !isWindows) {
                path = searchLibraryInPath("/usr/local/lib:/usr/lib:/usr/lib64", libName);
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
