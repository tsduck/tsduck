//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Representation of a "running" XML document which is displayed on the fly.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlDocument.h"

namespace ts {
    namespace xml {
        //!
        //! Representation of a "running" XML document which is displayed on the fly.
        //! @ingroup xml
        //!
        //! The idea is to display or save an XML document which is built element
        //! by element without waiting for the end of the document. Moreover,
        //! considering that the document can be arbitrary long, can take an arbitrary
        //! long time to be built and is not used for anything else than display or save,
        //! elements are destroyed after being displayed or saved to avoid wasting memory.
        //!
        class TSDUCKDLL RunningDocument: public Document
        {
            TS_NOCOPY(RunningDocument);
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //!
            explicit RunningDocument(Report& report = NULLREP);

            //!
            //! Destructor.
            //!
            virtual ~RunningDocument() override;

            //!
            //! Initialize the running document.
            //! The initial declaration and root are created.
            //! The output XML file is initialized but nothing is printed yet.
            //! @param [in] rootName Name of the root element to create.
            //! @param [in] declaration Optional XML declaration.
            //! When omitted or empty, the standard declaration is used, specifying UTF-8 as format.
            //! @param [in] fileName Output file name to create. When empty or "-", @a strm is used for output.
            //! @param [in,out] strm The default output text stream when @a fileName is empty or "-".
            //! The referenced stream object must remain valid as long as this object.
            //! @return New root element of the document or null on error.
            //!
            Element* open(const UString& rootName, const UString& declaration = UString(), const UString& fileName = UString(), std::ostream& strm = std::cout);

            //!
            //! Flush the running document.
            //! All elements under the document root are displayed or saved and then deleted.
            //! The XML document header is issued with the first element.
            //! The XML structure is left open for more elements, in the next call to flush().
            //!
            void flush();

            //!
            //! Close the running document.
            //! If the XML structure is still open, it is closed.
            //! The output file, if any, is closed.
            //!
            void close();

        private:
            TextFormatter _text;       // The text formatter.
            bool          _open_root;  // Document root has been printed and is left open.
        };
    }
}
