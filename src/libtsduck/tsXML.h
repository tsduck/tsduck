//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  XML utilities for TinyXML-2.
//!  All applications should use this header file instead of tinyxml.h.
//!
//!  - TinXML-2 home page: http://leethomason.github.io/tinyxml2/
//!  - TinXML-2 repository: https://github.com/leethomason/tinyxml2
//!  - TinXML-2 documentation: http://leethomason.github.io/tinyxml2/annotated.html
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNullReport.h"

// Definitions which are used by TinyXML-2.
#if defined(__windows) && defined(_TSDUCKDLL_IMPL) && !defined(TINYXML2_EXPORT)
    #define TINYXML2_EXPORT 1
#elif defined(__windows) && defined(_TSDUCKDLL_USE) && !defined(TINYXML2_IMPORT)
    #define TINYXML2_IMPORT 1
#endif

#include "tinyxml2.h"

namespace ts {
    //!
    //! XML utility class with error reporting.
    //!
    class TSDUCKDLL XML
    {
    public:
        //!
        //! Default constructor.
        //! @param [in,out] report Where to report errors. Default to null report.
        //! This report will be used to report all errors when using this object.
        //!
        explicit XML(ReportInterface& report = NULLREP);

        //!
        //! Load an XML file.
        //! @param [out] doc TinyXML document object to load.
        //! @param [in] fileName Name of the XML file to load.
        //! @param [in] search If true, use a search algorithm for the XML file:
        //! If @a fileName is not found and does not contain any directory part, search this file
        //! in the following places:
        //! - Directory of the current executable.
        //! - All directories in @c TSPLUGINS_PATH environment variable.
        //! - All directories in @c LD_LIBRARY_PATH environment variable (UNIX only).
        //! - All directories in @c PATH (UNIX) or @c Path (Windows) environment variable.
        //! @return True on success, false on error.
        //!
        bool loadDocument(tinyxml2::XMLDocument& doc, const std::string& fileName, bool search = true);

        //!
        //! Parse an XML document.
        //! @param [out] doc TinyXML document object to load.
        //! @param [in] xmlContent Content of the XML document.
        //! @return True on success, false on error.
        //!
        bool parseDocument(tinyxml2::XMLDocument& doc, const std::string& xmlContent);

        //!
        //! Report an error on the registered report interface.
        //! @param [in] message Application-specific error message.
        //! @param [in] code TinyXML error code.
        //! @param [in] node Optional node which triggered the error.
        //!
        void reportError(const std::string& message, tinyxml2::XMLError code = tinyxml2::XML_SUCCESS, tinyxml2::XMLNode* node = 0);

        //!
        //! Search a file.
        //! @param [in] fileName Name of the file to search.
        //! If @a fileName is not found and does not contain any directory part, search this file
        //! in the following places:
        //! - Directory of the current executable.
        //! - All directories in @c TSPLUGINS_PATH environment variable.
        //! - All directories in @c LD_LIBRARY_PATH environment variable (UNIX only).
        //! - All directories in @c PATH (UNIX) or @c Path (Windows) environment variable.
        //! @return The path to an existing file or an empty string if not found.
        //!
        static std::string SearchFile(const std::string& fileName);

    private:
        ReportInterface& _report;

        // Inaccessible operations.
        XML(const XML&) = delete;
        XML& operator=(const XML&) = delete;
    };
}
