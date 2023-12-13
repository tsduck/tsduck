//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generate file names based on counter or dates.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"

namespace ts {
    //!
    //! Generate file names based on counter or dates and time.
    //! @ingroup system
    //!
    //! An instance of this class is used when an application needs to generate
    //! multiple files based on a naming pattern and a counter or date and time.
    //!
    //! Counter pattern
    //! ---------------
    //! A name template is "base.ext". A number is automatically added to the name
    //! part so that successive files receive distinct names. Example: if the specified
    //! file name is base.ext, the various files are named base-000000.ext, base-000001.ext, etc.
    //! If the specified template already contains trailing digits, this unmodified
    //! name is used for the first file. Then, the integer part is incremented.
    //! Example: if the specified file name is base-027.ext, the various files are named
    //! base-027.ext, base-028.ext, etc.
    //!
    //! Date & time pattern
    //! -------------------
    //! Based on template "base.ext", the file names are base-YYYYMMDD-hhmmssmmm.ext where
    //! the date and time fields are optional, base on a ts::Time::FieldMask value.
    //!
    class TSDUCKDLL FileNameGenerator
    {
    public:
        //!
        //! Constructor.
        //! The initial state is counter mode with empty file template.
        //!
        FileNameGenerator() = default;

        //!
        //! Reinitialize the file name generator in counter mode.
        //! @param [in] name_template File name template.
        //! @param [in] initial_counter Initial value of the counter.
        //! Ignored if @a name_template already contains a counter value.
        //! @param [in] counter_width Width of the counter field in the file name.
        //! Ignored if @a name_template already contains a counter value.
        //!
        void initCounter(const fs::path& name_template, size_t initial_counter = 0, size_t counter_width = 6);

        //!
        //! Reinitialize the file name generator in date and time mode.
        //! @param [in] name_template File name template.
        //! @param [in] fields List of date and time fields to include in the file name.
        //! Ignored if @a name_template already contains a date and time value.
        //! @see Time::FieldMask
        //!
        void initDateTime(const fs::path& name_template, int fields = Time::DATETIME);

        //!
        //! Generate a new file name.
        //! In counter mode, the counter is incremented in the file name.
        //! In date and time mode, the current local time is used.
        //! @return A new file name.
        //!
        fs::path newFileName();

        //!
        //! Generate a new file name with a specific date and time.
        //! @param [in] time The reference time to use in date and time mode.
        //! Ignored in counter mode.
        //! @return A new file name.
        //!
        fs::path newFileName(const Time& time);

    private:
        UString _name_prefix {};               // Full name prefix.
        UString _name_suffix {};               // Full name suffix.
        bool    _counter_mode = true;          // Use counter mode (ie. not date and time).
        size_t  _counter_value = 0;            // Next counter value in file names.
        size_t  _counter_width = 6;            // Counter width in file name.
        int     _time_fields = Time::DATETIME; // The time fields to use.
        UString _last_time {};                 // The last returned time fields. Use _counter_value to avoid identical file names.

        // Initialize the name prefix and suffix. Return the number of trailing digits in prefix.
        size_t init(const fs::path& name_template);

        // Get the number of trailing digits in a string.
        static size_t TrailingDigits(const UString&);
    };
}
