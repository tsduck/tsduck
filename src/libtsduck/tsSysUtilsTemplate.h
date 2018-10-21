//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#pragma once

#if defined (TS_UNIX)
#include <glob.h>
#endif


//----------------------------------------------------------------------------
// Get all files matching a specified wildcard pattern and
// append them into a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::ExpandWildcardAndAppend(CONTAINER& container, const UString& pattern)
{
#if defined(TS_WINDOWS)

    // On Win32, FindFirstFile / FindNextFile return the file name without directory.
    // We keep the directory part in the pattern to add it later to all file names.
    const UString::size_type pos = pattern.rfind(PathSeparator);
    const UString dir(pos == NPOS ? u"" : pattern.substr(0, pos + 1));

    ::WIN32_FIND_DATAW fdata;

    // Initiate the search
    ::HANDLE handle = ::FindFirstFileW(pattern.wc_str(), &fdata);
    if (handle == INVALID_HANDLE_VALUE) {
        // No file matching the pattern is not an error
        const ErrorCode status = ::GetLastError();
        return status == SYS_SUCCESS || status == ERROR_FILE_NOT_FOUND;
    }

    // Loop on all file matching the pattern
    do {
        // Get next file name.
        fdata.cFileName[sizeof(fdata.cFileName) / sizeof(fdata.cFileName[0]) - 1] = 0;
        const UString file(reinterpret_cast<const UChar*>(fdata.cFileName));

        // Filter out . and ..
        if (file != u"." && file != u"..") {
            container.push_back(dir + file);
        }
    } while (::FindNextFileW(handle, &fdata) != 0);
    const ErrorCode status = ::GetLastError(); // FindNextFile status

    // Cleanup the search context
    ::FindClose(handle);
    return status == SYS_SUCCESS || status == ERROR_NO_MORE_FILES; // normal end of search

#elif defined(TS_UNIX)

    ::glob_t gl;
    ::memset(&gl, 0, sizeof (gl));
    int status = ::glob(pattern.toUTF8().c_str(), 0, nullptr, &gl);
    if (status == 0) {
        for (size_t n = 0; n < gl.gl_pathc; n++) {
            const UString file(UString::FromUTF8(gl.gl_pathv[n]));
            // Filter out . and ..
            if (file != u"." && file != u"..") {
                container.push_back(file);
            }
        }
    }
    ::globfree (&gl);
    return status == 0 || status == GLOB_NOMATCH;

#else
    #error "Unimplemented operating system"
#endif
}
