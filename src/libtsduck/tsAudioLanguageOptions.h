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
//
//  Options to update the language of an audio stream in a PMT.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsPMT.h"
#include "tsISO639LanguageDescriptor.h"

namespace ts {

    // Audio language options
    class TSDUCKDLL AudioLanguageOptions
    {
    public:
        // Default constructor
        AudioLanguageOptions();

        // Language code, must be a 3-character string.
        std::string getLanguageCode() const {return _language_code;}
        void setLanguageCode (const char* s) {_language_code = s; _language_code.resize (3, ' ');}
        void setLanguageCode (const std::string& s) {setLanguageCode (s.c_str());}

        // Audio type
        uint8_t getAudioType() const {return _audio_type;}
        void setAudioType (uint8_t t) {_audio_type = t;}

        // How to locate the audio stream.
        // First option: explicit PID value.
        bool locateByPID() const {return _audio_stream_number == 0;}
        PID getPID() const {return _audio_stream_number == 0 ? _pid : PID (PID_NULL);}
        void setPID (PID p) {_pid = p; _audio_stream_number = 0;}

        // Second option: Audio stream number in PMT.
        // Audio streams are numbered in increasing order of PID value.
        // The first audio stream is 1.
        bool locateByAudioStreamNumber() const {return _audio_stream_number != 0;}
        uint8_t getAudioStreamNumber() const {return _audio_stream_number;}
        void setAudioStreamNumber (uint8_t n) {_audio_stream_number = n;}

        // Build an ISO-639_language_descriptor from this object (cast operator)
        operator ISO639LanguageDescriptor() const {return ISO639LanguageDescriptor (_language_code, _audio_type);}

        // Assign from a command-line option. Syntax:
        //   language-code[:audio-type[:location]]
        // Errors are reported through the Args object.
        // Return true on success, false on error.
        bool getFromArgs (Args& args, const char* option_name, size_t index = 0);

        // Return a short parameter syntax
        static std::string GetSyntaxString() {return "language-code[:audio-type[:location]]";}

        // Return a help string for the parameter syntax
        static std::string GetHelpString();

    private:
        std::string _language_code;       // always 3-chars
        uint8_t       _audio_type;
        uint8_t       _audio_stream_number; // if zero, use PID.
        PID         _pid;
    };

    // Vector of audio language options
    class TSDUCKDLL AudioLanguageOptionsVector : public std::vector<AudioLanguageOptions>
    {
    public:
        typedef std::vector<AudioLanguageOptions> SuperClass;

        // Default constructor
        explicit AudioLanguageOptionsVector (size_type size = 0) : SuperClass (size) {}

        // Constructor from a list of command-line options.
        // Errors are reported through the Args object.
        AudioLanguageOptionsVector (Args& args, const char* option_name) : SuperClass() {getFromArgs (args, option_name);}

        // Assign from a list of command-line options.
        // Errors are reported through the Args object.
        // Return true on success, false on error.
        bool getFromArgs (Args& args, const char* option_name);

        // Apply requested transformations on a PMT.
        // Errors are reported through the specified report interface, using the specified severity.
        // Return true on success, false on error.
        bool apply (PMT&, ReportInterface&, int severity = Severity::Error) const;
    };
}
