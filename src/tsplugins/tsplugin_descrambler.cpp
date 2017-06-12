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
//  Transport stream processor shared library:
//  DVB-CSA (Common Scrambling Algorithm) Descrambler
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsScrambling.h"
#include "tsByteBlock.h"
#include "tsStringUtils.h"
#include "tsByteBlock.h"
#include "tsHexa.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DescramblerPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        DescramblerPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        Scrambling::EntropyMode        _cw_mode;  // CW entropy mode
        std::list<ByteBlock>           _cw_list;  // List of control words
        std::list<ByteBlock>::iterator _next_cw;  // Next control word
        Scrambling                     _key;      // Preprocessed current control word
        uint8_t                        _last_scv; // Scrambling_control_value in last packet

        // Inaccessible operations
        DescramblerPlugin() = delete;
        DescramblerPlugin(const DescramblerPlugin&) = delete;
        DescramblerPlugin& operator=(const DescramblerPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::DescramblerPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DescramblerPlugin::DescramblerPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "DVB descrambler using static control words.", "[options]"),
    _cw_mode(Scrambling::REDUCE_ENTROPY),
    _cw_list(),
    _next_cw(),
    _key(),
    _last_scv(0)
{
    option ("cw",                   'c', STRING);
    option ("cw-file",              'f', STRING);
    option ("no-entropy-reduction", 'n');

    setHelp ("Options:\n"
             "\n"
             "  -c value\n"
             "  --cw value\n"
             "      Specifies a fixed and constant control word for all TS packets.\n"
             "      The value must be a string of 16 hexadecimal digits.\n"
             "\n"
             "  -f name\n"
             "  --cw-file name\n"
             "      Specifies a text file containing the list of control words to apply.\n"
             "      Each line of the file must contain exactly 16 hexadecimal digits.\n"
             "      The next control word is used each time the \"scrambling_control\"\n"
             "      changes in the TS packets header.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --no-entropy-reduction\n"
             "      Do not perform CW entropy reduction to 48 bits. Keep full 64-bits CW.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::start()
{
    // Get entropy mode
    _cw_mode = present ("no-entropy-reduction") ? Scrambling::FULL_CW : Scrambling::REDUCE_ENTROPY;

    // Get control words as list of strings
    std::list<std::string> lines;
    if (present ("cw") + present ("cw-file") != 1) {
        tsp->error ("specify exactly one of --cw or --cw-file");
        return false;
    }
    else if (present ("cw-file")) {
        std::string file (value ("cw-file"));
        if (!LoadStrings (lines, file)) {
            tsp->error ("error loading file %s", file.c_str());
            return false;
        }
    }
    else {
        lines.push_back (value ("cw"));
    }

    // Decode control words from hexa to binary
    _cw_list.clear();
    ByteBlock cw;
    for (std::list<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
        Trim (*it);
        if (!it->empty()) {
            if (!HexaDecode (cw, *it) || cw.size() != CW_BYTES) {
                tsp->error ("invalid control word \"%s\" , specify 16 hexa digits", it->c_str());
                return false;
            }
            _cw_list.push_back (cw);
        }
    }
    if (_cw_list.empty()) {
        tsp->error ("no control word specified");
        return false;
    }
    tsp->verbose ("loaded %" FMT_SIZE_T "d control words", _cw_list.size());

    // Reset other states
    _last_scv = SC_CLEAR;
    _next_cw = _cw_list.end();

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DescramblerPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // If the packet has no payload, there is nothing to descramble
    if (!pkt.hasPayload()) {
        return TSP_OK;
    }

    // Get scrambling_control_value in packet.
    uint8_t scv = pkt.getScrambling();

    // Do not modify packet if not scrambled
    if (scv != SC_EVEN_KEY && scv != SC_ODD_KEY) {
        if (scv != SC_CLEAR) {
            tsp->debug ("invalid scrambling_control_value %d in PID 0x%04X", int (scv), int (pkt.getPID()));
        }
        return TSP_OK;
    }

    // Check if we need to select a new CW.
    if (_last_scv != scv) {
        // Point to next CW. Wrap to beginning at end of CW list.
        if (_next_cw == _cw_list.end()) {
            _next_cw = _cw_list.begin();
        }
        // Set key for DVB-CSA
        _key.init (_next_cw->data(), _cw_mode);
        tsp->log (Severity::Verbose, "using control word: " + Hexa (*_next_cw, hexa::SINGLE_LINE));
        // Point to next CW
        ++_next_cw;
        // Keep track of last scrambling_control_value
        _last_scv = scv;
    }

    // Descramble the packet payload
    _key.decrypt (pkt.getPayload(), pkt.getPayloadSize());

    // Reset scrambling_control_value to zero in TS header
    pkt.setScrambling (SC_CLEAR);

    return TSP_OK;
}
