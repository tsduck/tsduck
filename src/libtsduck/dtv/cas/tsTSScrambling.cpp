//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsTSScrambling.h"
#include "tsNames.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TSScrambling::TSScrambling(Report& report, uint8_t scrambling) :
    _report(report),
    _scrambling_type(scrambling),
    _next_cw(_cw_list.end()),
    _dvbcsa(),    // required on old gcc 10 and below (gcc bug)
    _dvbcissa(),  // required on old gcc 10 and below (gcc bug)
    _idsa(),      // required on old gcc 10 and below (gcc bug)
    _aescbc(),    // required on old gcc 10 and below (gcc bug)
    _aesctr()     // required on old gcc 10 and below (gcc bug)

{
    setScramblingType(scrambling);
}

ts::TSScrambling::TSScrambling(const TSScrambling& other) :
    BlockCipherAlertInterface(other),  // required on old gcc 8.5 and below (gcc bug)
    _report(other._report),
    _scrambling_type(other._scrambling_type),
    _explicit_type(other._explicit_type),
    _cw_list(other._cw_list),
    _next_cw(_cw_list.end()),
    _dvbcsa(),    // required on old gcc 10 and below (gcc bug)
    _dvbcissa(),  // required on old gcc 10 and below (gcc bug)
    _idsa(),      // required on old gcc 10 and below (gcc bug)
    _aescbc(),    // required on old gcc 10 and below (gcc bug)
    _aesctr()     // required on old gcc 10 and below (gcc bug)
{
    setScramblingType(_scrambling_type);
    _dvbcsa[0].setEntropyMode(other._dvbcsa[0].entropyMode());
    _dvbcsa[1].setEntropyMode(other._dvbcsa[1].entropyMode());
}

ts::TSScrambling::TSScrambling(TSScrambling&& other) :
    _report(other._report),
    _scrambling_type(other._scrambling_type),
    _explicit_type(other._explicit_type),
    _cw_list(other._cw_list),
    _next_cw(_cw_list.end()),
    _dvbcsa(),    // required on old gcc 10 and below (gcc bug)
    _dvbcissa(),  // required on old gcc 10 and below (gcc bug)
    _idsa(),      // required on old gcc 10 and below (gcc bug)
    _aescbc(),    // required on old gcc 10 and below (gcc bug)
    _aesctr()     // required on old gcc 10 and below (gcc bug)
{
    setScramblingType(_scrambling_type);
    _dvbcsa[0].setEntropyMode(other._dvbcsa[0].entropyMode());
    _dvbcsa[1].setEntropyMode(other._dvbcsa[1].entropyMode());
}


//----------------------------------------------------------------------------
// Force the usage of a specific algorithm.
//----------------------------------------------------------------------------

bool ts::TSScrambling::setScramblingType(uint8_t scrambling, bool overrideExplicit)
{
    if (overrideExplicit || !_explicit_type) {

        // Select the right pair of scramblers.
        switch (scrambling) {
            case SCRAMBLING_DVB_CSA1:
            case SCRAMBLING_DVB_CSA2:
                _scrambler[0] = &_dvbcsa[0];
                _scrambler[1] = &_dvbcsa[1];
                break;
            case SCRAMBLING_DVB_CISSA1:
                _scrambler[0] = &_dvbcissa[0];
                _scrambler[1] = &_dvbcissa[1];
                break;
            case SCRAMBLING_ATIS_IIF_IDSA:
                _scrambler[0] = &_idsa[0];
                _scrambler[1] = &_idsa[1];
                break;
            case SCRAMBLING_DUCK_AES_CBC:
                _scrambler[0] = &_aescbc[0];
                _scrambler[1] = &_aescbc[1];
                break;
            case SCRAMBLING_DUCK_AES_CTR:
                _scrambler[0] = &_aesctr[0];
                _scrambler[1] = &_aesctr[1];
                break;
            default:
                // Fallback to DVB-CSA2 if no scrambler was previously defined.
                if (_scrambler[0] == nullptr || _scrambler[1] == nullptr) {
                    _scrambling_type = SCRAMBLING_DVB_CSA2;
                    _scrambler[0] = &_dvbcsa[0];
                    _scrambler[1] = &_dvbcsa[1];
                }
                return false;
        }

        // Set scrambling type.
        if (_scrambling_type != scrambling) {
            _report.debug(u"switching scrambling type from %s to %s", {NameFromDTV(u"ScramblingMode", _scrambling_type), NameFromDTV(u"ScramblingMode", scrambling)});
            _scrambling_type = scrambling;
        }
    }

    // Make sure the current scramblers notify alerts to this object.
    _scrambler[0]->setAlertHandler(this);
    _scrambler[1]->setAlertHandler(this);
    _scrambler[0]->setCipherId(0);
    _scrambler[1]->setCipherId(1);
    return true;
}

void ts::TSScrambling::setEntropyMode(DVBCSA2::EntropyMode mode)
{
    _dvbcsa[0].setEntropyMode(mode);
    _dvbcsa[1].setEntropyMode(mode);
}

ts::DVBCSA2::EntropyMode ts::TSScrambling::entropyMode() const
{
    return (_scrambling_type == SCRAMBLING_DVB_CSA1 || _scrambling_type == SCRAMBLING_DVB_CSA2) ?
            _dvbcsa[0].entropyMode() : DVBCSA2::FULL_CW;
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSScrambling::defineArgs(Args &args)
{
    args.option(u"aes-cbc");
    args.help(u"aes-cbc",
              u"Use AES-CBC scrambling instead of DVB-CSA2 (the default). "
              u"The control words are 16-byte long instead of 8-byte. "
              u"The residue is left clear. "
              u"Specify a fixed initialization vector using the --iv option.\n\n"
              u"Note that this is a non-standard TS scrambling mode. "
              u"The only standard AES-based scrambling modes are ATIS-IDSA and DVB-CISSA "
              u"(DVB-CISSA is the same as AES-CBC with a DVB-defined IV). "
              u"The TSDuck scrambler automatically sets the scrambling_descriptor with "
              u"user-defined value " + UString::Hexa(uint8_t(SCRAMBLING_DUCK_AES_CBC)) + u".");

    args.option(u"aes-ctr");
    args.help(u"aes-ctr",
              u"Use AES-CTR scrambling instead of DVB-CSA2 (the default). "
              u"The control words are 16-byte long instead of 8-byte. "
              u"The residue is included in the scrambling. "
              u"Specify a fixed initialization vector using the --iv option. "
              u"See the option --ctr-counter-bits for the size of the counter part in the IV.\n\n"
              u"Note that this is a non-standard TS scrambling mode. "
              u"The only standard AES-based scrambling modes are ATIS-IDSA and DVB-CISSA. "
              u"The TSDuck scrambler automatically sets the scrambling_descriptor with "
              u"user-defined value " + UString::Hexa(uint8_t(SCRAMBLING_DUCK_AES_CTR)) + u".");

    args.option(u"atis-idsa");
    args.help(u"atis-idsa",
              u"Use ATIS-IDSA scrambling (ATIS-0800006) instead of DVB-CSA2 (the "
              u"default). The control words are 16-byte long instead of 8-byte.");

    args.option(u"iv", 0, Args::HEXADATA, 0, Args::UNLIMITED_COUNT, AES::BLOCK_SIZE, AES::BLOCK_SIZE);
    args.help(u"iv",
              u"With --aes-cbc or --aes-ctr, specifies a fixed initialization vector for all TS packets. "
              u"The value must be a string of 32 hexadecimal digits. "
              u"The default IV is all zeroes.");

    args.option(u"ctr-counter-bits", 0, Args::UNSIGNED);
    args.help(u"ctr-counter-bits",
              u"With --aes-ctr, specifies the size in bits of the counter part. "
              u"In the initialization vector, the fixed nounce part uses the first 128-N bits "
              u"and the counter part uses the last N bits. "
              u"By default, the counter part uses the second half of the IV (64 bits).");

    args.option(u"cw", 'c', Args::HEXADATA, 0, Args::UNLIMITED_COUNT, 8, 16);
    args.help(u"cw",
              u"Specifies a fixed and constant control word for all TS packets. The value "
              u"must be a string of 16 hexadecimal digits (32 digits with --atis-idsa).");

    args.option(u"cw-file", 'f', Args::FILENAME);
    args.help(u"cw-file", u"name",
              u"Specifies a text file containing the list of control words to apply. "
              u"Each line of the file must contain exactly 16 hexadecimal digits (32 "
              u"digits with --atis-idsa or --dvb-cissa). The next control word is used each time the "
              u"\"scrambling_control\" changes in the TS packets header. When all control "
              u"words are used, the first one is used again, and so on.");

    args.option(u"output-cw-file", 0, Args::FILENAME);
    args.help(u"output-cw-file", u"name",
              u"Specifies a text file to create. "
              u"Each line of the file will contain a control word in hexadecimal digits. "
              u"Each time a new control word is used to scramble or descramble packets, it is logged in the file. "
              u"The created file can be used later using --cw-file.");

    args.option(u"dvb-cissa");
    args.help(u"dvb-cissa",
              u"Use DVB-CISSA scrambling instead of DVB-CSA2 (the default). "
              u"The control words are 16-byte long instead of 8-byte.");

    args.option(u"dvb-csa2");
    args.help(u"dvb-csa2", u"Use DVB-CSA2 scrambling. This is the default.");

    args.option(u"no-entropy-reduction", 'n');
    args.help(u"no-entropy-reduction",
              u"With DVB-CSA2, do not perform control word entropy reduction to 48 bits. "
              u"Keep full 64-bit control words. Ignored with --atis-idsa or --dvb-cissa.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::TSScrambling::loadArgs(DuckContext& duck, Args& args)
{
    // Number of explicitly defined scrambling algorithms.
    const int algo_count =
        args.present(u"atis-idsa") +
        args.present(u"dvb-cissa") +
        args.present(u"dvb-csa2") +
        args.present(u"aes-cbc") +
        args.present(u"aes-ctr");

    // Set the scrambler to use.
    if (algo_count > 1) {
        args.error(u"--atis-idsa, --dvb-cissa, --dvb-csa2, --aes-cbc, --aes-ctr are mutually exclusive");
    }
    else if (args.present(u"atis-idsa")) {
        setScramblingType(SCRAMBLING_ATIS_IIF_IDSA);
    }
    else if (args.present(u"dvb-cissa")) {
        setScramblingType(SCRAMBLING_DVB_CISSA1);
    }
    else if (args.present(u"aes-cbc")) {
        setScramblingType(SCRAMBLING_DUCK_AES_CBC);
    }
    else if (args.present(u"aes-ctr")) {
        setScramblingType(SCRAMBLING_DUCK_AES_CTR);
    }
    else {
        setScramblingType(SCRAMBLING_DVB_CSA2);
    }

    // If an explicit scrambling type is given, the application should probably
    // ignore scrambling descriptors when descrambling.
    _explicit_type = algo_count > 0;

    // Set DVB-CSA2 entropy mode regardless of --atis-idsa or --dvb-cissa in case we switch later to DVB-CSA2.
    setEntropyMode(args.present(u"no-entropy-reduction") ? DVBCSA2::FULL_CW : DVBCSA2::REDUCE_ENTROPY);

    // Set AES-CBC/CTR initialization vector. The default is all zeroes.
    const ByteBlock iv(args.hexaValue(u"iv", ByteBlock(AES::BLOCK_SIZE, 0x00)));
    if (!_aescbc[0].setIV(iv.data(), iv.size()) ||
        !_aescbc[1].setIV(iv.data(), iv.size()) ||
        !_aesctr[0].setIV(iv.data(), iv.size()) ||
        !_aesctr[1].setIV(iv.data(), iv.size()))
    {
        args.error(u"error setting AES initialization vector");
    }

    // Set the size of the counter part with CTS mode.
    // The default is zero, meaning half nounce / half counter.
    const size_t counter_bits = args.intValue<size_t>(u"ctr-counter-bits");
    _aesctr[0].setCounterBits(counter_bits);
    _aesctr[1].setCounterBits(counter_bits);

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
    for (auto& it : lines) {
        it.trim();
        if (!it.empty()) {
            if (!it.hexaDecode(cw) || cw.size() != cwSize()) {
                args.error(u"invalid control word \"%s\", specify %d hexa digits", {it, 2 * cwSize()});
            }
            else {
                _cw_list.push_back(cw);
            }
        }
    }
    if (!_cw_list.empty()) {
        args.verbose(u"loaded %d control words", {_cw_list.size()});
    }

    // Name of the output file for control words.
    args.getValue(_out_cw_name, u"output-cw-file");

    return args.valid();
}


//----------------------------------------------------------------------------
// Start the scrambling session.
//----------------------------------------------------------------------------

bool ts::TSScrambling::start()
{
    bool success = true;

    // Point next CW to end of list. Will loop to first one.
    _next_cw = _cw_list.end();

    // Create the output file for control words.
    if (!_out_cw_name.empty()) {
        _out_cw_file.open(_out_cw_name.toUTF8().c_str(), std::ios::out);
        success = !_out_cw_file.fail();
        if (!success) {
            _report.error(u"error creating %s", {_out_cw_name});
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Stop the scrambling session.
//----------------------------------------------------------------------------

bool ts::TSScrambling::stop()
{
    // Close the output file for control words, if one was created.
    if (_out_cw_file.is_open()) {
        _out_cw_file.close();
    }
    return true;
}


//----------------------------------------------------------------------------
// Implementation of BlockCipherAlertInterface.
//----------------------------------------------------------------------------

bool ts::TSScrambling::handleBlockCipherAlert(BlockCipher& cipher, AlertReason reason)
{
    switch (reason) {
        case FIRST_ENCRYPTION:
        case FIRST_DECRYPTION: {
            // First usage of a new CW. Report it on debug and add it in --output-cw-file when necessary.
            ByteBlock key;
            cipher.getKey(key);
            if (!key.empty()) {
                const UString key_string(UString::Dump(key, UString::SINGLE_LINE));
                _report.debug(u"starting using CW %s (%s)", {key_string, cipher.cipherId() == 0 ? u"even" : u"odd"});
                if (_out_cw_file.is_open()) {
                    _out_cw_file << key_string << std::endl;
                }
            }
            return true;
        }
        case ENCRYPTION_EXCEEDED:
        case DECRYPTION_EXCEEDED:
        default: {
            // Not interested in other alerts.
            return true;
        }
    }
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
        _report.error(u"no fixed CW from command line");
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
    assert(algo != nullptr);

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

    // Select scrambling algo.
    assert(_encrypt_scv == SC_EVEN_KEY || _encrypt_scv == SC_ODD_KEY);
    CipherChaining* algo = _scrambler[_encrypt_scv & 1];
    assert(algo != nullptr);

    // Check if the residue shall be included in the scrambling.
    size_t psize = pkt.getPayloadSize();
    if (!algo->residueAllowed()) {
        // Remove the residue from the payload.
        assert(algo->blockSize() != 0);
        psize -= psize % algo->blockSize();
    }

    // Encrypt the packet.
    const bool ok = psize == 0 || algo->encryptInPlace(pkt.getPayload(), psize);
    if (ok) {
        pkt.setScrambling(_encrypt_scv);
    }
    else {
        _report.error(u"packet encryption error using %s", {algo->name()});
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

    // Select descrambling algo.
    CipherChaining* algo = _scrambler[_decrypt_scv & 1];
    assert(algo != nullptr);

    // Check if the residue shall be included in the scrambling.
    size_t psize = pkt.getPayloadSize();
    if (!algo->residueAllowed()) {
        // Remove the residue from the payload.
        assert(algo->blockSize() != 0);
        psize -= psize % algo->blockSize();
    }

    // Decrypt the packet.
    const bool ok = psize == 0 || algo->decryptInPlace(pkt.getPayload(), psize);
    if (ok) {
        pkt.setScrambling(SC_CLEAR);
    }
    else {
        _report.error(u"packet decryption error using %s", {algo->name()});
    }
    return ok;
}
