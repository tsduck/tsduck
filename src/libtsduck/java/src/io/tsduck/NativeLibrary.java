//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2024, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
