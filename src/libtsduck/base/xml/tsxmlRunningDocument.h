//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            Element* open(const UString& rootName, const UString& declaration = UString(), const fs::path& fileName = fs::path(), std::ostream& strm = std::cout);

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
            TextFormatter _text;      // The text formatter.
            bool _open_root = false;  // Document root has been printed and is left open.
        };
    }
}
