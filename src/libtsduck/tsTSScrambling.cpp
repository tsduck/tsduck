//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2018, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsTSScrambling.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TSScrambling::TSScrambling(Report& report, uint8_t scrambling) :
    _report(report),
    _scrambling_type(scrambling),
    _cw_list(),
    _next_cw(_cw_list.end()),
    _encrypt_scv(SC_CLEAR),
    _decrypt_scv(SC_CLEAR),
    _dvbcsa(),
    _idsa(),
    _scrambler{0, 0}
{
    setScramblingType(scrambling);
}

ts::TSScrambling::TSScrambling(const TSScrambling& other) :
    _report(other._report),
    _scrambling_type(other._scrambling_type),
    _cw_list(other._cw_list),
    _next_cw(_cw_list.end()),
    _encrypt_scv(SC_CLEAR),
    _decrypt_scv(SC_CLEAR),
    _dvbcsa(),
    _idsa(),
    _scrambler{0, 0}
{
    setScramblingType(_scrambling_type);
}


//----------------------------------------------------------------------------
// Force the usage of a specific algorithm.
//----------------------------------------------------------------------------

bool ts::TSScrambling::setScramblingType(uint8_t scrambling)
{
    switch (scrambling) {
        case SCRAMBLING_DVB_CSA2:
            _scrambler[0] = &_dvbcsa[0];
            _scrambler[1] = &_dvbcsa[1];
            break;
        case SCRAMBLING_ATIS_IIF_IDSA:
            _scrambler[0] = &_idsa[0];
            _scrambler[1] = &_idsa[1];
            break;
        default:
            // Fallback to DVB-CSA2 if no scrambler was previously defined.
            if (_scrambler[0] == 0 || _scrambler[1] == 0) {
                _scrambling_type = SCRAMBLING_DVB_CSA2;
                _scrambler[0] = &_dvbcsa[0];
                _scrambler[1] = &_dvbcsa[1];
            }
            return false;
    }

    _scrambling_type = scrambling;
    return true;
}

void ts::TSScrambling::setEntropyMode(DVBCSA2::EntropyMode mode)
{
    _dvbcsa[0].setEntropyMode(mode);
    _dvbcsa[1].setEntropyMode(mode);
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TSScrambling::addHelp(Args& args) const
{
    UString help =
        u"\n"
        u"Transport stream scrambling options:\n"
        u"\n"
        u"  --atis-idsa\n"
        u"      Use ATIS-IDSA descrambling (ATIS-0800006) instead of DVB-CSA2 (the\n"
        u"      default). The control words are 16-byte long instead of 8-byte.\n"
        u"\n"
        u"  -c value\n"
        u"  --cw value\n"
        u"      Specifies a fixed and constant control word for all TS packets. The value\n"
        u"      must be a string of 16 hexadecimal digits (32 digits with --atis-idsa).\n"
        u"\n"
        u"  -f name\n"
        u"  --cw-file name\n"
        u"      Specifies a text file containing the list of control words to apply.\n"
        u"      Each line of the file must contain exactly 16 hexadecimal digits (32\n"
        u"      digits with --atis-idsa). The next control word is used each time the\n"
        u"      \"scrambling_control\" changes in the TS packets header. When all control\n"
        u"      words are used, the first one is used again, and so on.\n"
        u"\n"
        u"  -n\n"
        u"  --no-entropy-reduction\n"
        u"      Do not perform CW entropy reduction to 48 bits. Keep full 64-bits CW.\n"
        u"      Ignored with --atis-idsa.\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSScrambling::defineOptions(Args& args) const
{
    args.option(u"atis-idsa",             0);
    args.option(u"cw",                   'c', Args::STRING);
    args.option(u"cw-file",              'f', Args::STRING);
    args.option(u"dvb-csa2",              0);
    args.option(u"no-entropy-reduction", 'n');
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::TSScrambling::loadArgs(Args& args)
{
    // Set the scrambler to use.
    if (args.present(u"atis-idsa") + args.present(u"dvb-csa2") > 1) {
        args.error(u"--atis-idsa and --dvb-csa2 are mutally exclusive");
    }
    else if (args.present(u"atis-idsa")) {
        setScramblingType(SCRAMBLING_ATIS_IIF_IDSA);
    }
    else {
        setScramblingType(SCRAMBLING_DVB_CSA2);
    }

    // Set DVB-CSA2 entropy mode regardless of --atis-idsa in case we switch later to DVB-CSA2.
    setEntropyMode(args.present(u"no-entropy-reduction") ? DVBCSA2::FULL_CW : DVBCSA2::REDUCE_ENTROPY);

    // Get control words as list of strings.
    UStringList lines;
    if (args.present(u"cw") + args.present(u"cw-file") > 1) {
        args.error(u"--cw and --cw-file are mutally exclusive");
    }
    else if (args.present(u"cw")) {
        lines.push_back(args.value(u"cw"));
    }
    else if (args.present(u"cw-file")) {
        const UString file(args.value(u"cw-file"));
        if (!UString::Load(lines, file)) {
            args.error(u"error loading file %s", {file});
        }
    }

    // Decode control words from hexa to binary
    _cw_list.clear();
    ByteBlock cw;
    for (UStringList::iterator it = lines.begin(); it != lines.end(); ++it) {
        it->trim();
        if (!it->empty()) {
            if (!it->hexaDecode(cw) || cw.size() != cwSize()) {
                args.error(u"invalid control word \"%s\" , specify %d hexa digits", {*it, 2 * cwSize()});
            }
            else {
                _cw_list.push_back(cw);
            }
        }
    }
    if (!_cw_list.empty()) {
        args.verbose(u"loaded %d control words", {_cw_list.size()});
    }

    // Point next CW to end of list.
    _next_cw = _cw_list.end();
    return args.valid();
}


//----------------------------------------------------------------------------
// Rewind the list of fixed control words. Will use first CW.
//----------------------------------------------------------------------------

void ts::TSScrambling::rewindFixedCW()
{
    _next_cw = _cw_list.end();
    _encrypt_scv = SC_CLEAR;
    _decrypt_scv = SC_CLEAR;
}


//----------------------------------------------------------------------------
// Set the next fixed control word as scrambling key.
//----------------------------------------------------------------------------

bool ts::TSScrambling::setNextFixedCW(int parity)
{
    // Error if no fixed control word were provided on the command line.
    if (_cw_list.empty()) {
        return false;
    }

    // Point to next CW.
    if (_next_cw != _cw_list.end()) {
        ++_next_cw;
    }
    if (_next_cw == _cw_list.end()) {
        _next_cw = _cw_list.begin();
    }
    assert(_next_cw != _cw_list.end());

    // Set the key in the descrambler.
    return setCW(*_next_cw, parity);
}


//----------------------------------------------------------------------------
// Set the control word for encrypt and decrypt.
//----------------------------------------------------------------------------

bool ts::TSScrambling::setCW(const ByteBlock& cw, int parity)
{
    CipherChaining* algo = _scrambler[parity & 1];
    assert(algo != 0);

    if (algo->setKey(cw.data(), cw.size())) {
        _report.debug(u"using scrambling key: " + UString::Dump(cw, UString::SINGLE_LINE));
        return true;
    }
    else {
        _report.error(u"error setting %d-byte key to %s", {cw.size(), algo->name()});
        return false;
    }
}


//----------------------------------------------------------------------------
// Set the parity of all subsequent encryptions.
//----------------------------------------------------------------------------

bool ts::TSScrambling::setEncryptParity(int parity)
{
    // Remember parity.
    const uint8_t previous_scv = _encrypt_scv;
    _encrypt_scv = SC_EVEN_KEY | (parity & 1);

    // In case of fixed control words, use next key when the parity changes.
    return !hasFixedCW() || _encrypt_scv == previous_scv || setNextFixedCW(_encrypt_scv);
}


//----------------------------------------------------------------------------
// Encrypt a TS packet with the current parity and corresponding CW.
//----------------------------------------------------------------------------

bool ts::TSScrambling::encrypt(TSPacket& pkt)
{
    // Filter out encrypted packets.
    if (pkt.isScrambled()) {
        _report.error(u"try to scramble an already scrambled packet");
        return false;
    }

    // Silently pass packets without payload.
    if (!pkt.hasPayload()) {
        return true;
    }

    // If no current parity is set, start with even by default.
    if (_encrypt_scv == SC_CLEAR && !setEncryptParity(SC_EVEN_KEY)) {
        return false;
    }

    // Encrypt the packet.
    CipherChaining* algo = _scrambler[_encrypt_scv & 1];

    assert(algo != 0);
    assert(_encrypt_scv == SC_EVEN_KEY || _encrypt_scv == SC_ODD_KEY);

    const bool ok = algo->encryptInPlace(pkt.getPayload(), pkt.getPayloadSize());
    if (ok) {
        pkt.setScrambling(_encrypt_scv);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Decrypt a TS packet with the CW corresponding to the parity in the packet.
//----------------------------------------------------------------------------

bool ts::TSScrambling::decrypt(TSPacket& pkt)
{
    // Clear or invalid packets are silently accepted.
    const uint8_t scv = pkt.getScrambling();
    if (scv != SC_EVEN_KEY && scv != SC_ODD_KEY) {
        return true;
    }

    // Update current parity.
    const uint8_t previous_scv = _decrypt_scv;
    _decrypt_scv = scv;

    // In case of fixed control word, use next key when the scrambling control changes.
    if (hasFixedCW() && previous_scv != _decrypt_scv && !setNextFixedCW(_decrypt_scv)) {
        return false;
    }

    // Decrypt the packet.
    CipherChaining* algo = _scrambler[_decrypt_scv & 1];
    assert(algo != 0);

    const bool ok = algo->decryptInPlace(pkt.getPayload(), pkt.getPayloadSize());
    if (ok) {
        pkt.setScrambling(SC_CLEAR);
    }
    return ok;
}
