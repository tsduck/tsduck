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
//  AES scrambling (experimental)
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsDecimal.h"
#include "tsHexa.h"
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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class AESPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        AESPlugin(TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        // Private data
        bool            _abort;           // Error (service not found, etc)
        bool            _descramble;      // Descramble instead of scramble
        Service         _service;         // Service name & id
        PIDSet          _scrambled;       // List of PID's to (de)scramble
        SectionDemux    _demux;           // Section demux
        ECB<AES>        _ecb;             // AES cipher in ECB mode
        CBC<AES>        _cbc;             // AES cipher in CBC mode
        CTS1<AES>       _cts1;            // AES cipher in CTS mode, RFC 2040 definition
        CTS2<AES>       _cts2;            // AES cipher in CTS mode, NIST definition
        CTS3<AES>       _cts3;            // AES cipher in ECB-CTS mode
        CTS4<AES>       _cts4;            // AES cipher in ECB-CTS mode (ST version)
        DVS042<AES>     _dvs042;          // AES cipher in DVS 042 mode
        CipherChaining* _chain;           // Selected cipher chaining mode

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&);

        // Process specific tables
        void processPAT(PAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);

        // Inaccessible operations
        AESPlugin() = delete;
        AESPlugin(const AESPlugin&) = delete;
        AESPlugin& operator=(const AESPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::AESPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AESPlugin::AESPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, "Experimental AES scrambling of TS packets.", "[options] [service]"),
    _abort(false),
    _descramble(false),
    _service(),
    _scrambled(),
    _demux(this),
    _ecb(),
    _cbc(),
    _cts1(),
    _cts2(),
    _cts3(),
    _cts4(),
    _dvs042(),
    _chain(0)
{
    option ("",            0,  STRING, 0, 1);
    option ("cbc",         0);
    option ("cts1",        0);
    option ("cts2",        0);
    option ("cts3",        0);
    option ("cts4",        0);
    option ("descramble", 'd');
    option ("dvs042",      0);
    option ("ecb",         0);
    option ("iv",         'i', STRING);
    option ("key",        'k', STRING, 1, 1);
    option ("pid",        'p', PIDVAL, 0, UNLIMITED_COUNT);

    setHelp ("Service:\n"
             "  Specifies the service to scramble. If the argument is an integer value\n"
             "  (either decimal or hexadecimal), it is interpreted as a service id.\n"
             "  Otherwise, it is interpreted as a service name, as specified in the SDT.\n"
             "  The name is not case sensitive and blanks are ignored. If the service is\n"
             "  unspecified, individual PID's are scrambled (see option --pid).\n"
             "\n"
             "Options:\n"
             "\n"
             "  --cbc\n"
             "      Use Cipher Block Chaining (CBC) mode without padding. The residue (last\n"
             "      part of the packet payload, shorter than 16 bytes) is left clear.\n"
             "\n"
             "  --cts1\n"
             "      Use Cipher Text Stealing (CTS) mode, as defined by Bruce Schneier in its\n"
             "      \"Applied Cryptography\" and by RFC 2040 as RC5-CTS. TS packets with a\n"
             "      payload shorter than 17 bytes are left clear.\n"
             "\n"
             "  --cts2\n"
             "      Use Cipher Text Stealing (CTS) mode as defined in NIST's proposal. TS\n"
             "      packets with a payload shorter than 16 bytes are left clear.\n"
             "\n"
             "  --cts3\n"
             "      Use ECB Cipher Text Stealing (ECB-CTS) mode, as defined by Wikipedia.\n"
             "      TS packets with a payload shorter than 17 bytes are left clear.\n"
             "\n"
             "  --cts4\n"
             "      Use (weird) ECB Cipher Text Stealing (ECB-CTS) mode, as implemented in\n"
             "      ST 71xx chips. TS packets with a payload shorter than 17 bytes are left\n"
             "      clear.\n"
             "\n"
             "  -d\n"
             "  --descramble\n"
             "      Descramble instead of scramble.\n"
             "\n"
             "  --dvs042\n"
             "      Use DVS 042 (now ANSI/SCTE 52 2003) cipher block chaining mode.\n"
             "      TS packets with a payload shorter than 16 bytes are left clear.\n"
             "\n"
             "  --ecb\n"
             "      Use Electronic Code Book (ECB) mode without padding. The residue (last\n"
             "      part of the packet payload, shorter than 16 bytes) is left clear.\n"
             "      This is the default mode.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -i value\n"
             "  --iv value\n"
             "      Specifies the initialization vector. Must be a string of 32 hexadecimal\n"
             "      digits. Must not be used in ECB mode. The default IV is all zeroes.\n"
             "\n"
             "  -k value\n"
             "  --key value\n"
             "      Specifies a fixed and constant AES key for all TS packets. The value\n"
             "      must be a string of 32 or 64 hexadecimal digits. This is a mandatory\n"
             "      parameter.\n"
             "\n"
             "  -p value\n"
             "  --pid value\n"
             "      Specifies a PID to scramble. Can be used instead of specifying a service.\n"
             "      Several -p or --pid options may be specified.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AESPlugin::start()
{
    // Get option values
    _descramble = present ("descramble");
    getPIDSet (_scrambled, "pid");
    if (present ("")) {
        _service.set (value (""));
    }

    // Get chaining mode.
    if (present ("ecb") + present ("cbc") + present ("cts1") + present ("cts2") + present ("cts3") + present ("cts4") + present ("dvs042") > 1) {
        tsp->error ("options --cbc, --cts1, --cts2, --cts3, --cts4, --dvs042 and --ecb are mutually exclusive");
        return false;
    }
    if (present ("cbc")) {
        _chain = &_cbc;
    }
    else if (present ("cts1")) {
        _chain = &_cts1;
    }
    else if (present ("cts2")) {
        _chain = &_cts2;
    }
    else if (present ("cts3")) {
        _chain = &_cts3;
    }
    else if (present ("cts4")) {
        _chain = &_cts4;
    }
    else if (present ("dvs042")) {
        _chain = &_dvs042;
    }
    else {
        _chain = &_ecb;
    }

    // Get AES key
    ByteBlock key;
    if (!HexaDecode (key, value ("key"))) {
        tsp->error ("invalid key, specify hexa digits");
        return false;
    }
    if (!_chain->isValidKeySize (key.size())) {
        tsp->error ("%" FMT_SIZE_T "d bytes is an invalid AES key size", key.size());
        return false;
    }
    if (!_chain->setKey (key.data(), key.size())) {
        tsp->error ("error in AES key schedule");
        return false;
    }
    tsp->verbose ("using " + Decimal (key.size() * 8) + " bits key: " + Hexa (key, hexa::SINGLE_LINE));

    // Get IV
    ByteBlock iv (_chain->minIVSize(), 0); // default IV is all zeroes
    if (present ("iv") && !HexaDecode (iv, value ("iv"))) {
        tsp->error ("invalid initialization vector, specify hexa digits");
        return false;
    }
    if (!_chain->setIV (iv.data(), iv.size())) {
        tsp->error ("incorrect initialization vector");
        return false;
    }
    if (iv.size() > 0) {
        tsp->verbose ("using " + Decimal (iv.size() * 8) + " bits IV: " + Hexa (iv, hexa::SINGLE_LINE));
    }

    // Initialize the demux
    // When the service id is known, we wait for the PAT. If it is not yet
    // known (only the service name is known), we wait for the SDT.
    _demux.reset();
    if (_service.hasId()) {
        _demux.addPID (PID_PAT);
    }
    else if (_service.hasName()) {
        _demux.addPID (PID_SDT);
    }

    // Reset other states
    _abort = false;

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::AESPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat (table);
                if (pat.isValid()) {
                    processPAT (pat);
                }
            }
            break;
        }
        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt (table);
                if (sdt.isValid()) {
                    processSDT (sdt);
                }
            }
            break;
        }
        case TID_PMT: {
            PMT pmt (table);
            if (pmt.isValid() && _service.hasId (pmt.service_id)) {
                processPMT (pmt);
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

void ts::AESPlugin::processSDT (SDT& sdt)
{
    // Look for the service by name
    assert (_service.hasName());
    uint16_t service_id;
    if (!sdt.findService (_service.getName(), service_id)) {
        tsp->error ("service \"" + _service.getName() + "\" not found in SDT");
        _abort = true;
        return;
    }

    // Remember service id
    _service.setId (service_id);
    _service.clearPMTPID();
    tsp->verbose ("found service id %d (0x%04X)", int (_service.getId()), int (_service.getId()));

    // No longer need the SDT, now need the PAT
    _demux.removePID (PID_SDT);
    _demux.addPID (PID_PAT);
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::AESPlugin::processPAT (PAT& pat)
{
    // Locate the service in the PAT
    assert (_service.hasId());
    const PAT::ServiceMap::iterator it = pat.pmts.find (_service.getId());

    // If service not found, error
    if (it == pat.pmts.end()) {
        tsp->error ("service %d (0x%04X) not found in PAT", int (_service.getId()), int (_service.getId()));
        _abort = true;
        return;
    }

    // Now filter the PMT
    _service.setPMTPID (it->second);
    _demux.addPID (it->second);
    tsp->verbose ("found PMT PID %d (0x%04X)", int (_service.getPMTPID()), int (_service.getPMTPID()));

    // No longer need the PAT
    _demux.removePID (PID_PAT);
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::AESPlugin::processPMT (PMT& pmt)
{
    // Loop on all elementary streams of the PMT.
    // Mark all video, audio and subtitles PIDs for scrambling
    _scrambled.reset();
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        if (it->second.isVideo() || it->second.isAudio() || it->second.isSubtitles()) {
            _scrambled.set(it->first);
            tsp->verbose ("scrambling PID %d (0x%04X)", int (it->first), int (it->first));
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AESPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Filter interesting sections
    _demux.feedPacket (pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Leave non-service or empty packets alone
    if (!_scrambled.test (pid) || !pkt.hasPayload()) {
        return TSP_OK;
    }

    // If packet to descramble is already clear, nothing to do
    if (_descramble && pkt.isClear()) {
        return TSP_OK;
    }

    // If packet to scramble is already scrambled, there is an error
    if (!_descramble && pkt.isScrambled()) {
        tsp->error ("PID %d (0x%04X) already scrambled", int (pid), int (pid));
        return TSP_END;
    }

    // Locate the packet payload
    uint8_t* pl = pkt.getPayload();
    size_t pl_size = pkt.getPayloadSize();
    if (!_chain->residueAllowed()) {
        // The chaining mode does not allow a residue.
        // Round the payload size down to a multiple of the block size.
        // Leave the residue clear.
        pl_size = RoundDown (pl_size, _chain->blockSize());
    }
    if (pl_size < _chain->minMessageSize()) {
        // The payload is too short to be scrambled, leave the packet clear
        return TSP_OK;
    }

    // Now (de)scramble the packet
    uint8_t tmp[PKT_SIZE];
    assert (pl_size < sizeof(tmp));
    if (_descramble) {
        if (!_chain->decrypt (pl, pl_size, tmp, pl_size)) {
            tsp->error ("AES decrypt error");
            return TSP_END;
        }
    }
    else {
        if (!_chain->encrypt (pl, pl_size, tmp, pl_size)) {
            tsp->error ("AES encrypt error");
            return TSP_END;
        }
    }
    ::memcpy (pl, tmp, pl_size);

    // Mark "even key" (there is only one key but we must set something).
    pkt.setScrambling(uint8_t(_descramble ? SC_CLEAR : SC_EVEN_KEY));

    return TSP_OK;
}
