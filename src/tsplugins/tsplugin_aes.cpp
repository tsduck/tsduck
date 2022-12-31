//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  AES scrambling (experimental)
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsIntegerUtils.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsAES.h"
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
        TS_NOBUILD_NOCOPY(AESPlugin);
    public:
        // Implementation of plugin API
        AESPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool            _descramble;      // Descramble instead of scramble
        Service         _service_arg;     // Service name & id
        PIDSet          _scrambled;       // List of PID's to (de)scramble
        ECB<AES>        _ecb;             // AES cipher in ECB mode
        CBC<AES>        _cbc;             // AES cipher in CBC mode
        CTS1<AES>       _cts1;            // AES cipher in CTS mode, RFC 2040 definition
        CTS2<AES>       _cts2;            // AES cipher in CTS mode, NIST definition
        CTS3<AES>       _cts3;            // AES cipher in ECB-CTS mode
        CTS4<AES>       _cts4;            // AES cipher in ECB-CTS mode (ST version)
        DVS042<AES>     _dvs042;          // AES cipher in DVS 042 mode
        CipherChaining* _chain;           // Selected cipher chaining mode

        // Working data:
        bool            _abort;           // Error (service not found, etc)
        Service         _service;         // Service name & id
        SectionDemux    _demux;           // Section demux

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
    ProcessorPlugin(tsp_, u"Experimental AES scrambling of TS packets", u"[options] [service]"),
    _descramble(false),
    _service_arg(),
    _scrambled(),
    _ecb(),
    _cbc(),
    _cts1(),
    _cts2(),
    _cts3(),
    _cts4(),
    _dvs042(),
    _chain(nullptr),
    _abort(false),
    _service(),
    _demux(duck, this)
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

    option(u"iv", 'i', HEXADATA, 0, Args::UNLIMITED_COUNT, AES::BLOCK_SIZE, AES::BLOCK_SIZE);
    help(u"iv",
         u"Specifies the initialization vector. Must be a string of 32 hexadecimal "
         u"digits. Must not be used in ECB mode. The default IV is all zeroes.");

    option(u"key", 'k', HEXADATA, 1, 1, AES::MIN_KEY_SIZE, AES::MAX_KEY_SIZE);
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

    // Get chaining mode.
    if (present(u"ecb") + present(u"cbc") + present(u"cts1") + present(u"cts2") + present(u"cts3") + present(u"cts4") + present(u"dvs042") > 1) {
        tsp->error(u"options --cbc, --cts1, --cts2, --cts3, --cts4, --dvs042 and --ecb are mutually exclusive");
        return false;
    }
    if (present(u"cbc")) {
        _chain = &_cbc;
    }
    else if (present(u"cts1")) {
        _chain = &_cts1;
    }
    else if (present(u"cts2")) {
        _chain = &_cts2;
    }
    else if (present(u"cts3")) {
        _chain = &_cts3;
    }
    else if (present(u"cts4")) {
        _chain = &_cts4;
    }
    else if (present(u"dvs042")) {
        _chain = &_dvs042;
    }
    else {
        _chain = &_ecb;
    }

    // Get AES key
    const ByteBlock key(hexaValue(u"key"));
    if (!_chain->isValidKeySize(key.size())) {
        tsp->error(u"%d bytes is an invalid AES key size", {key.size()});
        return false;
    }
    if (!_chain->setKey(key.data(), key.size())) {
        tsp->error(u"error in AES key schedule");
        return false;
    }
    tsp->verbose(u"using %d bits key: %s", {key.size() * 8, UString::Dump(key, UString::SINGLE_LINE)});

    // Get IV, default IV is all zeroes
    const ByteBlock iv(hexaValue(u"iv", ByteBlock(_chain->minIVSize(), 0)));
    if (!_chain->setIV(iv.data(), iv.size())) {
        tsp->error(u"incorrect initialization vector");
        return false;
    }
    tsp->verbose(u"using %d bits IV: %s", {iv.size() * 8, UString::Dump(iv, UString::SINGLE_LINE)});

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
        tsp->error(u"service \"%s\" not found in SDT", {_service.getName()});
        _abort = true;
        return;
    }

    // Remember service id
    _service.setId(service_id);
    _service.clearPMTPID();
    tsp->verbose(u"found service id %d (0x%X)", {_service.getId(), _service.getId()});

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
        tsp->error(u"service %d (0x%X) not found in PAT", {_service.getId(), _service.getId()});
        _abort = true;
        return;
    }

    // Now filter the PMT
    _service.setPMTPID(it->second);
    _demux.addPID(it->second);
    tsp->verbose(u"found PMT PID %d (0x%X)", {_service.getPMTPID(), _service.getPMTPID()});

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
            tsp->verbose(u"scrambling PID %d (0x%X)", {it.first, it.first});
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
        tsp->error(u"PID %d (0x%X) already scrambled", {pid, pid});
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
            tsp->error(u"AES decrypt error");
            return TSP_END;
        }
    }
    else {
        if (!_chain->encrypt(pl, pl_size, tmp, pl_size)) {
            tsp->error(u"AES encrypt error");
            return TSP_END;
        }
    }
    ::memcpy(pl, tmp, pl_size);

    // Mark "even key" (there is only one key but we must set something).
    pkt.setScrambling(uint8_t(_descramble ? SC_CLEAR : SC_EVEN_KEY));

    return TSP_OK;
}
