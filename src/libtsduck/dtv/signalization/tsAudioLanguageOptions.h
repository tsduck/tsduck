//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Options to update the language of an audio stream in a PMT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsPMT.h"
#include "tsISO639LanguageDescriptor.h"

namespace ts {
    //!
    //! Options to update the language of an audio stream in a PMT.
    //! @ingroup mpeg
    //!
    //! This class is a helper for commands and tools which manipulate
    //! languages in a PMT.
    //!
    //! On a command line, the value of an audion option is "language-code[:audio-type[:location]]".
    //!
    //! The "language-code" is a 3-character string. The audio-type is optional,
    //! its default value is zero. The "location" indicates how to locate the
    //! audio stream. Its format is either "Pn" or "An". In the first case,
    //! "n" designates a PID value and in the second case the audio stream number.
    //! inside the PMT, starting with 1. The default location is "A1", ie. the
    //! first audio stream inside the PMT.
    //! Audio streams are numbered in increasing order of PID value.
    //!
    class TSDUCKDLL AudioLanguageOptions
    {
    public:
        //!
        //! Default constructor.
        //!
        AudioLanguageOptions() = default;

        //!
        //! Get the language code.
        //! @return The language code, a 3-character string.
        //!
        UString getLanguageCode() const
        {
            return _language_code;
        }

        //!
        //! Set the language code.
        //! @param [in] s The language code, must be a 3-character string.
        //!
        void setLanguageCode(const UString& s)
        {
            _language_code = s;
            _language_code.resize(3, u' ');
        }

        //!
        //! Get the audio type.
        //! @return The audio type.
        //!
        uint8_t getAudioType() const
        {
            return _audio_type;
        }

        //!
        //! Set the audio type.
        //! @param [in] t The audio type.
        //!
        void setAudioType(uint8_t t)
        {
            _audio_type = t;
        }

        //!
        //! Check how to locate the audio stream.
        //! @return True when the audio stream is located by PID value, false when
        //! it is located by stream number inside the PMT.
        //!
        bool locateByPID() const
        {
            return _audio_stream_number == 0;
        }

        //!
        //! Get audio PID.
        //! @return The audio PID number or @link PID_NULL @endlink if the audio stream
        //! is defined by index inside the PMT.
        //!
        PID getPID() const
        {
            return _audio_stream_number == 0 ? _pid : PID(PID_NULL);
        }

        //!
        //! Set audio PID.
        //! @param [in] p The audio PID number.
        //!
        void setPID(PID p)
        {
            _pid = p;
            _audio_stream_number = 0;
        }

        //!
        //! Get the audio stream number in PMT.
        //! @return The audio stream number in PMT. The first audio stream is 1.
        //!
        uint8_t getAudioStreamNumber() const
        {
            return _audio_stream_number;
        }

        //!
        //! Set the audio stream number in PMT.
        //! @param [in] n The audio stream number in PMT. The first audio stream is 1.
        //!
        void setAudioStreamNumber(uint8_t n)
        {
            _audio_stream_number = n == 0 ? 1 : n;
        }

        //!
        //! Build an ISO-639_language_descriptor from this object (cast operator).
        //! @return An ISO639LanguageDescriptor object for this audio description.
        //!
        operator ISO639LanguageDescriptor() const
        {
            return ISO639LanguageDescriptor(_language_code, _audio_type);
        }

        //!
        //! Assign from a command-line option.
        //! Syntax of the option value: language-code[:audio-type[:location]]
        //! @param [in,out] args Command line arguments. Errors are also reported through the @a args object.
        //! @param [in] option_name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return True on success, false on error.
        //!
        bool getFromArgs(Args& args, const UChar* option_name, size_t index = 0);

        //!
        //! Return a short parameter syntax.
        //! @return A short parameter syntax.
        //!
        static UString GetSyntaxString()
        {
            return u"language-code[:audio-type[:location]]";
        }

        //!
        //! Return a help string for the parameter syntax.
        //! @return A multi-line help string for the parameter syntax.
        //!
        static UString GetHelpString();

    private:
        UString _language_code {3, SPACE};  // always 3-chars
        uint8_t _audio_type = 0;
        uint8_t _audio_stream_number {1};   // use first audio stream by default, use PID if zero
        PID     _pid = PID_NULL;
    };

    //!
    //! Vector of audio language options.
    //!
    class TSDUCKDLL AudioLanguageOptionsVector : public std::vector<AudioLanguageOptions>
    {
    public:
        //!
        //! Explicit naming of superclass.
        //!
        typedef std::vector<AudioLanguageOptions> SuperClass;

        //!
        //! Default constructor.
        //! @param [in] size Initial number of elements in the list. Default: empty.
        //!
        explicit AudioLanguageOptionsVector(size_type size = 0) :
            SuperClass(size)
        {
        }

        //!
        //! Constructor from a list of command-line options.
        //! @param [in,out] args Command line arguments. Errors are also reported through the @a args object.
        //! @param [in] option_name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported. This object is populated with one element per occurrence of
        //! the option.
        //!
        AudioLanguageOptionsVector(Args& args, const UChar* option_name) :
            SuperClass()
        {
            getFromArgs(args, option_name);
        }

        //!
        //! Assign from a list of command-line options.
        //! @param [in,out] args Command line arguments. Errors are also reported through the @a args object.
        //! @param [in] option_name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported. This object is populated with one element per occurrence of
        //! the option.
        //! @return True on success, false on error.
        //!
        bool getFromArgs(Args& args, const UChar* option_name);

        //!
        //! Apply requested transformations on a PMT.
        //! @param [in,out] duck TSDuck execution environment.
        //! @param [in,out] pmt The PMT to update. All audio streams are modified according to the
        //! content of this vector of audio options.
        //! @param [in] severity Severity of errors (ts::Severity::Error by default).
        //! @return True on success, false on error.
        //!
        bool apply(DuckContext& duck, PMT& pmt, int severity = Severity::Error) const;
    };
}
