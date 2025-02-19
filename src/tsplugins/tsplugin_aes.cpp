//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  AES scrambling (experimental)
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsIntegerUtils.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsAES128.h"
#include "tsAES256.h"
#include "tsECB.h"
#include "tsCBC.h"
#include "tsCTS1.h"
#include "tsCTS2.h"
#include "tsCTS3.h"
#include "tsCTS4.h"
#include "tsDVS042.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class AESPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(AESPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using CipherPtr = std::shared_ptr<BlockCipher>;

        // Command line options:
        bool      _descramble = false; // Descramble instead of scramble
        Service   _service_arg {};     // Service name & id
        PIDSet    _scrambled {};       // List of PID's to (de)scramble
        CipherPtr _chain {};           // Selected cipher chaining mode

        // Working data:
        bool         _abort = false;      // Error (service not found, etc)
        Service      _service {};         // Service name & id
        SectionDemux _demux {duck, this}; // Section demux

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(PAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"aes", ts::AESPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AESPlugin::AESPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Experimental AES scrambling of TS packets", u"[options] [service]")
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 0, 1);
    help(u"",
         u"Specifies the service to scramble. If the argument is an integer value "
         u"(either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored. If the service is "
         u"unspecified, individual PID's are scrambled (see option --pid).");

    option(u"cbc");
    help(u"cbc",
         u"Use Cipher Block Chaining (CBC) mode without padding. The residue (last "
         u"part of the packet payload, shorter than 16 bytes) is left clear. ");

    option(u"cts1");
    help(u"cts1",
         u"Use Cipher Text Stealing (CTS) mode, as defined by Bruce Schneier in its "
         u"\"Applied Cryptography\" and by RFC 2040 as RC5-CTS. TS packets with a "
         u"payload shorter than 17 bytes are left clear.");

    option(u"cts2");
    help(u"cts2",
         u"Use Cipher Text Stealing (CTS) mode as defined in NIST's proposal. TS "
         u"packets with a payload shorter than 16 bytes are left clear.");

    option(u"cts3");
    help(u"cts3",
         u"Use ECB Cipher Text Stealing (ECB-CTS) mode, as defined by Wikipedia. "
         u"TS packets with a payload shorter than 17 bytes are left clear.");

    option(u"cts4");
    help(u"cts4",
         u"Use (weird) ECB Cipher Text Stealing (ECB-CTS) mode, as implemented in ST 71xx chips. "
         u"TS packets with a payload shorter than 17 bytes are left clear.");

    option(u"descramble", 'd');
    help(u"descramble",
         u"Descramble instead of scramble.");

    option(u"dvs042");
    help(u"dvs042",
         u"Use DVS 042 (now ANSI/SCTE 52 2003) cipher block chaining mode. "
         u"TS packets with a payload shorter than 16 bytes are left clear.");

    option(u"ecb");
    help(u"ecb",
         u"Use Electronic Code Book (ECB) mode without padding. The residue (last "
         u"part of the packet payload, shorter than 16 bytes) is left clear. "
         u"This is the default mode.");

    option(u"iv", 'i', HEXADATA, 0, Args::UNLIMITED_COUNT, AES128::BLOCK_SIZE, AES128::BLOCK_SIZE);
    help(u"iv",
         u"Specifies the initialization vector. Must be a string of 32 hexadecimal digits. "
         u"Must not be used in ECB, CTS3, CTS4 modes. The default IV is all zeroes.");

    option(u"key", 'k', HEXADATA, 1, 1, AES128::KEY_SIZE, AES256::KEY_SIZE);
    help(u"key",
         u"Specifies a fixed and constant AES key for all TS packets. The value "
         u"must be a string of 32 or 64 hexadecimal digits. This is a mandatory "
         u"parameter.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specifies a PID to scramble. Can be used instead of specifying a service. "
         u"Several -p or --pid options may be specified.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::AESPlugin::getOptions()
{
    duck.loadArgs(*this);
    _descramble = present(u"descramble");
    getIntValues(_scrambled, u"pid");
    if (present(u"")) {
        _service_arg.set(value(u""));
    }

    // Get key and chaining mode.
    const ByteBlock key(hexaValue(u"key"));
    if (present(u"ecb") + present(u"cbc") + present(u"cts1") + present(u"cts2") + present(u"cts3") + present(u"cts4") + present(u"dvs042") > 1) {
        error(u"options --cbc, --cts1, --cts2, --cts3, --cts4, --dvs042 and --ecb are mutually exclusive");
        return false;
    }
    if (present(u"cbc")) {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new CBC<AES128>) : CipherPtr(new CBC<AES256>);
    }
    else if (present(u"cts1")) {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new CTS1<AES128>) : CipherPtr(new CTS1<AES256>);
    }
    else if (present(u"cts2")) {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new CTS2<AES128>) : CipherPtr(new CTS2<AES256>);
    }
    else if (present(u"cts3")) {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new CTS3<AES128>) : CipherPtr(new CTS3<AES256>);
    }
    else if (present(u"cts4")) {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new CTS4<AES128>) : CipherPtr(new CTS4<AES256>);
    }
    else if (present(u"dvs042")) {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new DVS042<AES128>) : CipherPtr(new DVS042<AES256>);
    }
    else {
        _chain = key.size() == AES128::KEY_SIZE ? CipherPtr(new ECB<AES128>) : CipherPtr(new ECB<AES256>);
    }

    // Get AES key
    if (!_chain->isValidKeySize(key.size())) {
        error(u"%d bytes is an invalid AES key size", key.size());
        return false;
    }
    if (!_chain->setKey(key.data(), key.size())) {
        error(u"error in AES key schedule");
        return false;
    }
    verbose(u"using %d bits key: %s", key.size() * 8, UString::Dump(key, UString::SINGLE_LINE));

    // Get IV, default IV is all zeroes
    const ByteBlock iv(hexaValue(u"iv", ByteBlock(_chain->minIVSize(), 0)));
    if (!_chain->setIV(iv.data(), iv.size())) {
        error(u"incorrect initialization vector");
        return false;
    }
    verbose(u"using %d bits IV: %s", iv.size() * 8, UString::Dump(iv, UString::SINGLE_LINE));

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AESPlugin::start()
{
    // Initialize the demux
    // When the service id is known, we wait for the PAT. If it is not yet
    // known (only the service name is known), we wait for the SDT.
    _demux.reset();
    if (_service_arg.hasId()) {
        _demux.addPID(PID_PAT);
    }
    else if (_service_arg.hasName()) {
        _demux.addPID(PID_SDT);
    }

    // Reset other states.
    _service = _service_arg;
    _abort = false;

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::AESPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(duck, table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }
        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt(duck, table);
                if (sdt.isValid()) {
                    processSDT(sdt);
                }
            }
            break;
        }
        case TID_PMT: {
            PMT pmt(duck, table);
            if (pmt.isValid() && _service.hasId(pmt.service_id)) {
                processPMT(pmt);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Service Description Table (SDT).
//  We search the service in the SDT. Once we get the service, we rebuild a
//  new SDT containing only one section and only one service (a copy of
//  all descriptors for the service).
//----------------------------------------------------------------------------

void ts::AESPlugin::processSDT(SDT& sdt)
{
    // Look for the service by name
    assert(_service.hasName());
    uint16_t service_id;
    if (!sdt.findService(duck, _service.getName(), service_id)) {
        error(u"service \"%s\" not found in SDT", _service.getName());
        _abort = true;
        return;
    }

    // Remember service id
    _service.setId(service_id);
    _service.clearPMTPID();
    verbose(u"found service id %n", _service.getId());

    // No longer need the SDT, now need the PAT
    _demux.removePID(PID_SDT);
    _demux.addPID(PID_PAT);
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::AESPlugin::processPAT(PAT& pat)
{
    // Locate the service in the PAT
    assert(_service.hasId());
    auto it = pat.pmts.find(_service.getId());

    // If service not found, error
    if (it == pat.pmts.end()) {
        error(u"service %n not found in PAT", _service.getId());
        _abort = true;
        return;
    }

    // Now filter the PMT
    _service.setPMTPID(it->second);
    _demux.addPID(it->second);
    verbose(u"found PMT PID %n", _service.getPMTPID());

    // No longer need the PAT
    _demux.removePID(PID_PAT);
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::AESPlugin::processPMT(PMT& pmt)
{
    // Loop on all elementary streams of the PMT.
    // Mark all video, audio and subtitles PIDs for scrambling
    _scrambled.reset();
    for (const auto& it : pmt.streams) {
        if (it.second.isVideo(duck) || it.second.isAudio(duck) || it.second.isSubtitles(duck)) {
            _scrambled.set(it.first);
            verbose(u"scrambling PID %n", it.first);
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AESPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Filter interesting sections
    _demux.feedPacket(pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Leave non-service or empty packets alone
    if (!_scrambled.test(pid) || !pkt.hasPayload()) {
        return TSP_OK;
    }

    // If packet to descramble is already clear, nothing to do
    if (_descramble && pkt.isClear()) {
        return TSP_OK;
    }

    // If packet to scramble is already scrambled, there is an error
    if (!_descramble && pkt.isScrambled()) {
        error(u"PID %n already scrambled", pid);
        return TSP_END;
    }

    // Locate the packet payload
    uint8_t* pl = pkt.getPayload();
    size_t pl_size = pkt.getPayloadSize();
    if (!_chain->residueAllowed()) {
        // The chaining mode does not allow a residue.
        // Round the payload size down to a multiple of the block size.
        // Leave the residue clear.
        pl_size = round_down(pl_size, _chain->blockSize());
    }
    if (pl_size < _chain->minMessageSize()) {
        // The payload is too short to be scrambled, leave the packet clear
        return TSP_OK;
    }

    // Now (de)scramble the packet
    uint8_t tmp[PKT_SIZE];
    assert (pl_size < sizeof(tmp));
    if (_descramble) {
        if (!_chain->decrypt(pl, pl_size, tmp, pl_size)) {
            error(u"AES decrypt error");
            return TSP_END;
        }
    }
    else {
        if (!_chain->encrypt(pl, pl_size, tmp, pl_size)) {
            error(u"AES encrypt error");
            return TSP_END;
        }
    }
    MemCopy(pl, tmp, pl_size);

    // Mark "even key" (there is only one key but we must set something).
    pkt.setScrambling(uint8_t(_descramble ? SC_CLEAR : SC_EVEN_KEY));

    return TSP_OK;
}
