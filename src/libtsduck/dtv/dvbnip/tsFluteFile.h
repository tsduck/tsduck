//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a file extracted from a FLUTE stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsFluteSessionId.h"

namespace ts {
    //!
    //! Representation of a file extracted from a FLUTE stream.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteFile
    {
        TS_NOBUILD_NOCOPY(FluteFile);
    public:
        //!
        //! Constructor.
        //! @param [in] sid Session id.
        //! @param [in] toi Transport Object Identifier.
        //! @param [in] name File name or URN.
        //! @param [in] type File MIME type.
        //! @param [in] content File content.
        //!
        FluteFile(const FluteSessionId& sid,
                  uint64_t              toi,
                  const UString&        name,
                  const UString&        type,
                  const ByteBlockPtr&   content);

        //!
        //! Get the session id of the file.
        //! @return A constant reference to the ssession in the object.
        //!
        const FluteSessionId& sessionId() const { return _sid; }

        //!
        //! Get the Transport Object Identifier (TOI) of the file.
        //! @return The Transport Object Identifier.
        //!
        uint64_t toi() const { return _toi; }

        //!
        //! Get the name or URN of the file.
        //! @return A constant reference to the name in the object.
        //!
        const UString& name() const { return _name; }

        //!
        //! Get the MIME type of the file.
        //! @return A constant reference to the type string in the object.
        //!
        const UString& type() const { return _type; }

        //!
        //! Get the size of the file.
        //! @return The size of the file in bytes.
        //!
        size_t size() const { return _content->size(); }

        //!
        //! Access the content of the file (modifiable version).
        //! @return A reference to the content of the file.
        //!
        ByteBlock& content() { return *_content; }

        //!
        //! Access the content of the file (constant version).
        //! @return A constant reference to the content of the file.
        //!
        const ByteBlock& content() const { return *_content; }

        //!
        //! Get a character string version of the file, if it is a text file.
        //! @return A string resulting from the conversion of the file content from UTF-8.
        //!
        UString toText() const;

        //!
        //! Get an indented XML character string version of the file, if it is a text file.
        //! @return A string resulting from the conversion of the file content from UTF-8 and XML reindentation.
        //! If the text is not XML, the original text is returned.
        //!
        UString toXML() const;

    private:
        FluteSessionId _sid;
        uint64_t       _toi;
        UString        _name;
        UString        _type;
        ByteBlockPtr   _content;
    };
}
