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
//!  Representation of an parental_rating_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an parental_rating_descriptor
    //! @see ETSI 300 468, 6.2.28.
    //!
    class TSDUCKDLL ParentalRatingDescriptor : public AbstractDescriptor
    {
    public:
        // Item entry
        struct Entry;

        //!
        //! A list of item entries.
        //!
        typedef std::list<Entry> EntryList;

        // Public members
        EntryList entries;  //!< The list of item entries.

        //!
        //! Default constructor.
        //!
        ParentalRatingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ParentalRatingDescriptor(const Descriptor& bin);

        //!
        //! Constructor with one entry.
        //! @param [in] language ISO-639 language code, 3 characters.
        //! @param [in] rating Parental rating.
        //!
        ParentalRatingDescriptor(const std::string& language, uint8_t rating);

        // Inherited methods
        virtual void serialize(Descriptor&) const;
        virtual void deserialize(const Descriptor&);

        //!
        //! Item entry/
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            std::string language_code;  //!< ISO-639 language code, 3 characters.
            uint8_t     rating;         //!< Parental rating.

            //!
            //! Constructor.
            //! @param [in] language_ ISO-639 language code, 3 characters.
            //! @param [in] rating_ Parental rating.
            //!
            Entry(const std::string& language_ = "", uint8_t rating_ = 0) :
                language_code(language_),
                rating(rating_)
            {
            }
        };
    };
}
