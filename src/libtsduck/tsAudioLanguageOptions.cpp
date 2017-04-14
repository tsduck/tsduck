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

#include "tsAudioLanguageOptions.h"
#include "tsToInteger.h"



//----------------------------------------------------------------------------
// Return a help string for the parameter syntax (static method)
//----------------------------------------------------------------------------

std::string ts::AudioLanguageOptions::GetHelpString()
{
    return
        "      The \"language-code\" is a 3-character string. The audio-type is optional,\n"
        "      its default value is zero. The \"location\" indicates how to locate the\n"
        "      audio stream. Its format is either \"Pn\" or \"An\". In the first case,\n"
        "      \"n\" designates a PID value and in the second case the audio stream number\n"
        "      inside the PMT, starting with 1. The default location is \"A1\", ie. the\n"
        "      first audio stream inside the PMT.\n";
}


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::AudioLanguageOptions::AudioLanguageOptions() :
    _language_code (3, ' '),  // always 3-chars
    _audio_type (0),
    _audio_stream_number (1), // use first audio stream by default
    _pid (PID_NULL)
{
}


//----------------------------------------------------------------------------
// Assign from a command-line option.
//----------------------------------------------------------------------------

bool ts::AudioLanguageOptions::getFromArgs (Args& args, const char* option_name, size_t index)
{
    // Get parameter value
    const std::string val (args.value (option_name, "", index));
    const size_t len = val.length();

    // Must be at least 3-chars long
    if (len < 3 || len == 4) {
        goto error;
    }

    // Get default values
    _language_code = val.substr (0, 3);
    _audio_type = 0;
    _audio_stream_number = 1;
    _pid = PID_NULL;

    // Get additional info
    if (len > 3) {
        // Get ":audio_type"
        if (val[3] != ':') {
            goto error;
        }
        size_t col = val.find (":", 4);
        if (col == std::string::npos) {
            col = len;
        }
        else {
            // Found ":location"
            if (col < 5 || col + 2 >= len) {
                goto error;
            }
            uint16_t x = 0;
            const char type = val[col + 1];
            if ((type != 'P' && type != 'p' && type != 'A' && type != 'a') || !ToInteger (x, val.substr (col + 2, len - col - 2))) {
                goto error;
            }
            if ((type == 'P' || type == 'p') && x < PID_MAX) {
                _pid = PID (x);
                _audio_stream_number = 0;
            }
            else if ((type == 'A' || type == 'a') && x > 0 && x <= 0xFF) {
                _pid = PID_NULL;
                _audio_stream_number = uint8_t (x);
            }
            else {
                goto error;
            }
        }
        // Decode audio_type
        if (!ToInteger (_audio_type, val.substr (4, col - 4))) {
            goto error;
        }
    }
    
    return true;

 error:
    const std::string syntax (GetSyntaxString());
    args.error ("invalid value \"%s\" for option --%s, use %s", val.c_str(), option_name, syntax.c_str());
    return false;
}


//----------------------------------------------------------------------------
// Assign from a list of command-line options.
//----------------------------------------------------------------------------

bool ts::AudioLanguageOptionsVector::getFromArgs (Args& args, const char* option_name)
{
    clear();
    AudioLanguageOptions opt;
    for (size_t n = 0; n < args.count (option_name); n++) {
        if (!opt.getFromArgs (args, option_name, n)) {
            return false;
        }
        push_back (opt);
    }
    return true;
}


//----------------------------------------------------------------------------
// Apply requested transformations on a PMT.
//----------------------------------------------------------------------------

bool ts::AudioLanguageOptionsVector::apply (PMT& pmt, ReportInterface& report, int severity) const
{
    bool ok = true;
    // Loop on all options
    for (const_iterator it = begin(); it != end(); ++it) {
        PMT::StreamMap::iterator smi;
        // Find audio stream in PMT
        if (it->locateByPID()) {
            // Find the audio stream by PID in the PMT
            smi = pmt.streams.find (it->getPID());
            if (smi == pmt.streams.end()) {
                report.log (severity, "audio PID %d (0x%04X) not found in PMT", int (it->getPID()), int (it->getPID()));
                ok = false;
            }
        }
        else {
            // Find audio stream number in PMT
            assert (it->getAudioStreamNumber() != 0);
            size_t audio_count = 0;
            smi = pmt.streams.begin();
            while (smi != pmt.streams.end()) {
                if (smi->second.isAudio() && ++audio_count >= it->getAudioStreamNumber()) {
                    break;
                }
                ++smi;
            }
            if (smi == pmt.streams.end()) {
                report.log (severity, "audio stream %d not found in PMT", int (it->getAudioStreamNumber()));
                ok = false;
            }
        }
        // Update audio stream in PMT
        if (smi != pmt.streams.end()) {
            // Remove any previous language descriptor
            smi->second.descs.removeByTag (DID_LANGUAGE);
            // Add a new one
            smi->second.descs.add (ISO639LanguageDescriptor (*it));
        }
    }
    return ok;
}
