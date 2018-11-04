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
//!
//!  @file
//!  HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshls.h"
#include "tsCerrReport.h"
#include "tsWebRequestArgs.h"

namespace ts {
    namespace hls {
        //!
        //! Playlist for HTTP Live Streaming (HLS).
        //! @ingroup hls
        //!
        class TSDUCKDLL PlayList
        {
        public:
            //!
            //! Constructor.
            //!
            PlayList();

            //!
            //! Clear the content of the playlist.
            //!
            void clear();

            //!
            //! Load the playlist from a URL.
            //! @param [in] url URL from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] args Web request arguments from command line.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadURL(const UString& url, bool strict = false, const WebRequestArgs args = WebRequestArgs(), Report& report = CERR);

            //!
            //! Load the playlist from a text file.
            //! @param [in] filename File from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadFile(const UString& filename, bool strict = false, Report& report = CERR);

            //!
            //! Load the playlist from its text content.
            //! @param [in] text Text of the playlist (multi-lines).
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadText(const UString& text, bool strict = false, Report& report = CERR);

            //!
            //! Check if the playlist has been successfully loaded.
            //! @return True if the playlist has been successfully loaded.
            //!
            bool isValid() const { return _valid; }

            //!
            //! Get the original URL.
            //! @return The original URL.
            //!
            UString url() const { return _url; }

            //!
            //! Build an URL for a media segment or sub playlist.
            //! @param [in] uri An URI, typically extracted from the playlist.
            //! @return The complete URL.
            //!
            UString buildURL(const UString& uri) const;

            //!
            //! Get the playlist type.
            //! @return The playlist type.
            //!
            PlayListType type() const { return _type; }

            //!
            //! Get the playlist version (EXT-X-VERSION).
            //! @return The playlist version.
            //!
            int version() const { return _version; }

        private:
            bool         _valid;     // Content loaded and valid.
            int          _version;   // Playlist format version.
            PlayListType _type;      // Playlist type.
            UString      _url;       // Original URL (or file name).
            UString      _urlBase;   // Base URL (to resolve relative URI's).
            bool         _isURL;     // The base is an URL, not a directory name.

            // Load from the text content.
            bool parse(const UString& text, bool strict, Report& report);
            bool parse(const UStringList& lines, bool strict, Report& report);

            // Set the playlist type, return true on success, false on error.
            bool setType(PlayListType type, Report& report);
        };
    }
}
