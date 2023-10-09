//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAudioLanguageOptions.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Return a help string for the parameter syntax (static method)
//----------------------------------------------------------------------------

ts::UString ts::AudioLanguageOptions::GetHelpString()
{
    return
        u"The \"language-code\" is a 3-character string. The audio-type is optional, "
        u"its default value is zero. The \"location\" indicates how to locate the "
        u"audio stream. Its format is either \"Pn\" or \"An\". In the first case, "
        u"\"n\" designates a PID value and in the second case the audio stream number "
        u"inside the PMT, starting with 1. The default location is \"A1\", ie. the "
        u"first audio stream inside the PMT.\n";
}


//----------------------------------------------------------------------------
// Assign from a command-line option.
//----------------------------------------------------------------------------

bool ts::AudioLanguageOptions::getFromArgs(Args& args, const UChar* option_name, size_t index)
{
    // Get parameter value
    const UString val(args.value(option_name, u"", index));
    const size_t len = val.length();

    // Must be at least 3-chars long
    if (len < 3 || len == 4) {
        goto error;
    }

    // Get default values
    _language_code = val.substr(0, 3);
    _audio_type = 0;
    _audio_stream_number = 1;
    _pid = PID_NULL;

    // Get additional info
    if (len > 3) {
        // Get ":audio_type"
        if (val[3] != u':') {
            goto error;
        }
        size_t col = val.find(u":", 4);
        if (col == NPOS) {
            col = len;
        }
        else {
            // Found ":location"
            if (col < 5 || col + 2 >= len) {
                goto error;
            }
            uint16_t x = 0;
            const UChar type = val[col + 1];
            if ((type != u'P' && type != u'p' && type != u'A' && type != u'a') || !val.substr(col + 2, len - col - 2).toInteger(x)) {
                goto error;
            }
            if ((type == u'P' || type == u'p') && x < PID_MAX) {
                _pid = PID(x);
                _audio_stream_number = 0;
            }
            else if ((type == u'A' || type == u'a') && x > 0 && x <= 0xFF) {
                _pid = PID_NULL;
                _audio_stream_number = uint8_t(x);
            }
            else {
                goto error;
            }
        }
        // Decode audio_type
        if (!val.substr(4, col - 4).toInteger(_audio_type)) {
            goto error;
        }
    }

    return true;

 error:
    args.error(u"invalid value \"%s\" for option --%s, use %s", {val, option_name, GetSyntaxString()});
    return false;
}


//----------------------------------------------------------------------------
// Assign from a list of command-line options.
//----------------------------------------------------------------------------

bool ts::AudioLanguageOptionsVector::getFromArgs(Args& args, const UChar* option_name)
{
    clear();
    AudioLanguageOptions opt;
    for (size_t n = 0; n < args.count(option_name); n++) {
        if (!opt.getFromArgs(args, option_name, n)) {
            return false;
        }
        push_back(opt);
    }
    return true;
}


//----------------------------------------------------------------------------
// Apply requested transformations on a PMT.
//----------------------------------------------------------------------------

bool ts::AudioLanguageOptionsVector::apply(DuckContext& duck, PMT& pmt, int severity) const
{
    bool ok = true;
    // Loop on all options
    for (const_iterator it = begin(); it != end(); ++it) {
        auto smi = pmt.streams.end();
        // Find audio stream in PMT
        if (it->locateByPID()) {
            // Find the audio stream by PID in the PMT
            smi = pmt.streams.find(it->getPID());
            if (smi == pmt.streams.end()) {
                duck.report().log(severity, u"audio PID %d (0x%X) not found in PMT", {it->getPID(), it->getPID()});
                ok = false;
            }
        }
        else {
            // Find audio stream number in PMT
            assert(it->getAudioStreamNumber() != 0);
            size_t audio_count = 0;
            smi = pmt.streams.begin();
            while (smi != pmt.streams.end()) {
                if (smi->second.isAudio(duck) && ++audio_count >= it->getAudioStreamNumber()) {
                    break;
                }
                ++smi;
            }
            if (smi == pmt.streams.end()) {
                duck.report().log(severity, u"audio stream %d not found in PMT", {it->getAudioStreamNumber()});
                ok = false;
            }
        }
        // Update audio stream in PMT
        if (smi != pmt.streams.end()) {
            // Remove any previous language descriptor
            smi->second.descs.removeByTag(DID_LANGUAGE);
            // Add a new one
            smi->second.descs.add(duck, ISO639LanguageDescriptor(*it));
        }
    }
    return ok;
}
