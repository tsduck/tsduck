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
//  Abstract base class for DVB descrambler plugins.
//
//----------------------------------------------------------------------------

#include "tsAbstractDescrambler.h"
#include "tsGuardCondition.h"
#include "tsDecimal.h"
#include "tsHexa.h"
#include "tsFormat.h"
TSDUCK_SOURCE;

#define ECM_THREAD_STACK_OVERHEAD (16  * 1024)  // Stack usage in this module
#define ECM_THREAD_STACK_USAGE    (128 * 1024)  // Default stack usage for CAS


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractDescrambler::AbstractDescrambler (TSP* tsp_,
                                                const std::string& description_,
                                                const std::string& syntax_,
                                                const std::string& help_) :
    ProcessorPlugin (tsp_, description_, syntax_, help_),
    _cw_mode (Scrambling::REDUCE_ENTROPY),
    _packet_count (0),
    _abort (false),
    _synchronous (false),
    _aes128_dvs042 (false),
    _iv (),
    _service (),
    _stack_usage (ECM_THREAD_STACK_USAGE),
    _demux (this),
    _ecm_streams (),
    _scrambled_streams (),
    _mutex (),
    _ecm_to_do (),
    _stop_thread (false)
{
}


//----------------------------------------------------------------------------
// Get the ECM stream for a PID, create it if non existent
//----------------------------------------------------------------------------

ts::AbstractDescrambler::ECMStreamPtr ts::AbstractDescrambler::getOrCreateECMStream (PID ecm_pid)
{
    ECMStreamMap::iterator ecm_it = _ecm_streams.find (ecm_pid);
    if (ecm_it != _ecm_streams.end()) {
        return ecm_it->second;
    }
    else {
        ECMStreamPtr p (new ECMStream());
        _ecm_streams.insert (std::make_pair (ecm_pid, p));
        return p;
    }
}


//----------------------------------------------------------------------------
// Start abstract descrambler.
//----------------------------------------------------------------------------

bool ts::AbstractDescrambler::startDescrambler (bool           synchronous,
                                                  bool           reduce_entropy,
                                                  const Service& service,
                                                  size_t         stack_usage)
{
    // Get descrambler parameters
    _cw_mode = reduce_entropy ? Scrambling::REDUCE_ENTROPY : Scrambling::FULL_CW;
    _synchronous = synchronous;
    _service = service;
    _stack_usage = stack_usage > 0 ? stack_usage : ECM_THREAD_STACK_USAGE;

    // Reset descrambler state
    _abort = false;
    _ecm_streams.clear();
    _scrambled_streams.clear();

    // Initialize the section demux.
    // If the service is known by name, filter the SDT, otherwise filter the PAT.
    _demux.reset();
    _demux.addPID(PID(_service.hasName() ? PID_SDT : PID_PAT));

    // In asynchronous mode, create a thread for ECM processing
    if (!_synchronous) {
        _stop_thread = false;
        ThreadAttributes attr;
        Thread::getAttributes(attr);
        attr.setStackSize(ECM_THREAD_STACK_OVERHEAD + _stack_usage);
        Thread::setAttributes(attr);
        Thread::start();
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop abstract descrambler.
//----------------------------------------------------------------------------

bool ts::AbstractDescrambler::stop()
{
    // In asynchronous mode, notify the ECM processing thread to terminate
    // and wait for its actual termination.
    if (!_synchronous) {
        {
            GuardCondition lock (_mutex, _ecm_to_do);
            _stop_thread = true;
            lock.signal();
        }
        Thread::waitForTermination();
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat (table);
            if (pat.isValid()) {
                processPAT (pat);
            }
            break;
        }

        case TID_SDT_ACT: {
            SDT sdt (table);
            if (sdt.isValid()) {
                processSDT (sdt);
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

        case TID_ECM_80:
        case TID_ECM_81: {
            if (table.sectionCount() == 1) {
                processCMT (*table.sectionAt(0));
            }
            break;
        }

        default: {
            // Not interested in other tables.
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Service Description Table (SDT).
//  We search the service in the SDT. Once we get the service, we rebuild a
//  new SDT containing only one section and only one service (a copy of
//  all descriptors for the service).
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::processSDT(const SDT& sdt)
{
    // Look for the service by name
    uint16_t service_id;
    assert(_service.hasName());
    if (!sdt.findService(_service.getName(), service_id)) {
        tsp->error("service \"" + _service.getName() + "\" not found in SDT");
        _abort = true;
        return;
    }

    // Remember service id
    _service.setId(service_id);
    tsp->verbose("found service \"" + _service.getName() + Format("\", service id is 0x%04X", int(_service.getId())));

    // No longer need to filter the SDT
    _demux.removePID(PID_SDT);

    // Now filter the PAT to get the PMT PID
    _demux.addPID(PID_PAT);
    _service.clearPMTPID();
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::processPAT (const PAT& pat)
{
    if (_service.hasId()) {
        // The service id is known, search it in the PAT
        PAT::ServiceMap::const_iterator it = pat.pmts.find (_service.getId());
        if (it == pat.pmts.end()) {
            // Service not found, error
            tsp->error ("service id %d (0x%04X) not found in PAT", int (_service.getId()), int (_service.getId()));
            _abort = true;
            return;
        }
        // If a previous PMT PID was known, no long filter it
        if (_service.hasPMTPID()) {
            _demux.removePID (_service.getPMTPID());
        }
        // Found PMT PID
        _service.setPMTPID (it->second);
        _demux.addPID (it->second);
    }
    else if (!pat.pmts.empty()) {
        // No service specified, use first one in PAT
        PAT::ServiceMap::const_iterator it = pat.pmts.begin();
        _service.setId (it->first);
        _service.setPMTPID (it->second);
        _demux.addPID (it->second);
        tsp->verbose ("using service %d (0x%04X)", int (_service.getId()), int (_service.getId()));
    }
    else {
        // No service specified, no service in PAT, error
        tsp->error ("no service in PAT");
        _abort = true;
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::processPMT (const PMT& pmt)
{
    tsp->debug ("PMT: service 0x%04X, %" FMT_SIZE_T "d elementary streams", int (pmt.service_id), pmt.streams.size());

    // Search ECM PID's at service level
    std::set<PID> service_ecm_pids;
    analyzeCADescriptors (pmt.descs, service_ecm_pids);

    // Loop on all elementary streams in this service
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        const PID pid = it->first;
        const PMT::Stream& stream (it->second);

        // Search ECM PIDs at elementary stream level.
        std::set<PID> component_ecm_pids;
        analyzeCADescriptors (stream.descs, component_ecm_pids);

        // If none found as stream level, use the ones from service level.
        if (!component_ecm_pids.empty()) {
            _scrambled_streams[pid].ecm_pids = component_ecm_pids;
        }
        else if (!service_ecm_pids.empty()) {
            _scrambled_streams[pid].ecm_pids = service_ecm_pids;
        }
    }
}


//----------------------------------------------------------------------------
// Analyze a list of descriptors, looking for ECM PID's
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::analyzeCADescriptors (const DescriptorList& dlist, std::set<PID>& ecm_pids)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search (DID_CA); index < dlist.count(); index = dlist.search (DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // The fixed part of a CA descriptor is 4 bytes long.
        if (size < 4) {
            continue;
        }
        uint16_t sysid = GetUInt16 (desc);
        uint16_t pid = GetUInt16 (desc + 2) & 0x1FFF;
        desc += 4; size -= 4;

        // Ask subclass if this PID is OK
        if (checkCADescriptor (sysid, desc, size)) {
            ecm_pids.insert (pid);
            getOrCreateECMStream (pid);
            _demux.addPID (pid);
            tsp->verbose ("using ECM PID %d (0x%04X)", int (pid), int (pid));
        }

        // Normally, no PID should be referenced in the private part of a CA
        // descriptor. However, this rule is not followed by MediaGuard.
        if (CASFamilyOf (sysid) == CAS_MEDIAGUARD && size >= 13) {
            desc += 13; size -= 13;
            while (size >= 15) {
                pid = GetUInt16 (desc) & 0x1FFF;
                if (checkCADescriptor (sysid, desc + 2, 13)) {
                    ecm_pids.insert (pid);
                    getOrCreateECMStream (pid);
                    _demux.addPID (pid);
                    tsp->verbose ("using ECM PID %d (0x%04X)", int (pid), int (pid));
                }
                desc += 15; size -= 15;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process one CMT (CA Message Table) section containing an ECM
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::processCMT (const Section& sect)
{
    const PID ecm_pid = sect.sourcePID();
    tsp->log (2, "got ECM (TID 0x%02X) on PID %d (0x%04X)", int (sect.tableId()), int (ecm_pid), int (ecm_pid));

    // Get ECM stream context
    ECMStreamMap::iterator ecm_it = _ecm_streams.find (ecm_pid);
    if (ecm_it == _ecm_streams.end()) {
        tsp->warning ("got ECM on non-ECM PID %d (0x%04X)", int (ecm_pid), int (ecm_pid));
        return;
    }
    ECMStreamPtr& estream (ecm_it->second);

    // If same TID as previous CMT on this PID, give up, this is the same ECM.
    if (sect.tableId() == estream->last_tid) {
        return;
    }

    // This is a new ECM on this PID.
    estream->last_tid = sect.tableId();

    // Check if the ECM can be deciphered (ask subclass)
    if (!checkECM (sect.payload(), sect.payloadSize())) {
        tsp->log (2, "ECM not handled by subclass");
        return;
    }

    if (sect.payloadSize() > sizeof(estream->ecm)) {
        tsp->error ("ECM too long (%" FMT_SIZE_T "d bytes) on PID %d (0x%04X)",
                    sect.payloadSize(), int (ecm_pid), int (ecm_pid));
        return;
    }

    tsp->debug ("new ECM (TID 0x%02X) on PID %d (0x%04X)", int (sect.tableId()), int (ecm_pid), int (ecm_pid));

    // In asynchronous mode, the CW are accessed under mutex protection.
    if (!_synchronous) {
        _mutex.acquire();
    }

    // Copy the ECM into the PID context.
    // Flawfinder: ignore: memcpy()
    ::memcpy(estream->ecm, sect.payload(), sect.payloadSize());
    estream->ecm_size = sect.payloadSize();
    estream->new_ecm = true;

    // Decipher the ECM.
    if (_synchronous) {
        // Synchronous mode: directly decipher the ECM
        processECM(*estream);
    }
    else {
        // Asynchronous mode: signal the ECM to the ECM processing thread.
        _ecm_to_do.signal();
        _mutex.release();
    }
}


//----------------------------------------------------------------------------
// Process one ECM (the one in ECMStream::ecm).
// In asynchronous mode, this method must be invoked with the mutex held.
// Release the mutex while deciphering the ECM and relock it before exiting.
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::processECM (ECMStream& estream)
{
    // Copy the ECM out of the protected area into local data

    uint8_t ecm [MAX_PSI_SECTION_SIZE];
    size_t ecm_size = estream.ecm_size;
    assert(estream.ecm_size <= sizeof(ecm));
    ::memcpy(ecm, estream.ecm, estream.ecm_size);  // Flawfinder: ignore: memcpy()
    estream.new_ecm = false;

    // In asynchronous mode, release the mutex.

    if (!_synchronous) {
        _mutex.release();
    }

    // Here, we have an ECM to decipher.

    tsp->debug("packet %" FMT_INT64 "d, decipher ECM, %" FMT_SIZE_T "d bytes: %02X %02X %02X %02X %02X %02X %02X %02X ...",
               _packet_count - 1, ecm_size,
               int(ecm[0]), int(ecm[1]), int(ecm[2]), int(ecm[3]),
               int(ecm[4]), int(ecm[5]), int(ecm[6]), int(ecm[7]));

    // Submit the ECM to the CAS (subclass)

    uint8_t cw_even [CW_BYTES];
    uint8_t cw_odd [CW_BYTES];
    bool ok = decipherECM(ecm, ecm_size, cw_even, cw_odd);

    if (ok) {
        tsp->debug("even CW: %02X %02X %02X %02X %02X %02X %02X %02X",
                   int(cw_even[0]), int(cw_even[1]), int(cw_even[2]), int(cw_even[3]),
                   int(cw_even[4]), int(cw_even[5]), int(cw_even[6]), int(cw_even[7]));
        tsp->debug("odd CW:  %02X %02X %02X %02X %02X %02X %02X %02X",
                   int(cw_odd[0]), int(cw_odd[1]), int(cw_odd[2]), int(cw_odd[3]),
                   int(cw_odd[4]), int(cw_odd[5]), int(cw_odd[6]), int(cw_odd[7]));
    }

    // In asynchronous mode, relock the mutex.

    if (!_synchronous) {
        _mutex.acquire();
    }

    // Copy the control words in the protected area.
    // Normally, only one CW is modified for each new ECM.
    // Compare extracted CW with previous ones to avoid signaling a new
    // CW when it is actually unchanged.

    if (ok) {
        if (!estream.cw_valid || ::memcmp (estream.cw_even, cw_even, CW_BYTES) != 0) {
            // Previous even CW was either invalid or different from new one
            estream.new_cw_even = true;
            ::memcpy(estream.cw_even, cw_even, CW_BYTES);  // Flawfinder: ignore: memcpy()
        }
        if (!estream.cw_valid || ::memcmp (estream.cw_odd, cw_odd, CW_BYTES) != 0) {
            // Previous odd CW was either invalid or different from new one
            estream.new_cw_odd = true;
            ::memcpy(estream.cw_odd, cw_odd, CW_BYTES);  // Flawfinder: ignore: memcpy()
        }
    }

    estream.cw_valid = ok;
}


//----------------------------------------------------------------------------
// ECM deciphering thread
//----------------------------------------------------------------------------

void ts::AbstractDescrambler::main()
{
    tsp->debug ("ECM processing thread started");

    // ECM processing loop.
    // The loop executes with the mutex held. The mutex is released
    // while deciphering an ECM and while waiting for the condition
    // variable 'ecm_to_do'.

    GuardCondition lock (_mutex, _ecm_to_do);

    for (;;) {

        // Look for an ECM to decipher. Loop as long as ECM's are
        // found (or a terminate request is encountered). We loop
        // again if at least an ECM was found since the mutex was
        // released during the ECM processing and a new ECM may
        // have been added at the beginning of the list.

        bool got_ecm, terminate;

        do {
            got_ecm = false;
            terminate = _stop_thread;

            // Decipher ECM's on all ECM PID's.
            for (ECMStreamMap::iterator it = _ecm_streams.begin(); !terminate && it != _ecm_streams.end(); ++it) {
                ECMStreamPtr& estream (it->second);
                if (estream->new_ecm) {

                    // Found an ECM, decipher it. Note that the mutex is
                    // released while deciphering the ECM.
                    got_ecm = true;
                    processECM (*estream);

                    // Look for termination request while deciphering
                    terminate = _stop_thread;
                }
            }
        } while (!terminate && got_ecm);

        // Check if a terminate request is found
        if (terminate) {
            break;
        }

        // We have accomplished a full scan of all ECM PID's and found no ECM.
        // The mutex was consequently not released during the loop and we are
        // now sure that there is nothing to do. The mutex is implicitely
        // released and we wait for the condition 'ecm_to_do' and, once we
        // get it, implicitely relock the mutex.
        lock.waitCondition();
    }

    tsp->debug ("ECM processing thread terminated");
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AbstractDescrambler::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Count packets
    _packet_count++;

    // Filter interesting sections
    _demux.feedPacket (pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // If the packet has no payload, there is nothing to descramble
    if (!pkt.hasPayload()) {
        return TSP_OK;
    }

    // Get scrambling_control_value in packet.
    uint8_t scv = pkt.getScrambling();

    // Do not modify packet if not scrambled
    if (scv != SC_EVEN_KEY && scv != SC_ODD_KEY) {
        return TSP_OK;
    }

    // Get PID context. If the PID is not known as a scrambled PID,
    // with a corresponding ECM stream, we cannot descramble it.
    const PID pid = pkt.getPID();
    ScrambledStreamMap::iterator ssit = _scrambled_streams.find (pid);
    if (ssit == _scrambled_streams.end()) {
        return TSP_OK;
    }
    ScrambledStream& ss (ssit->second);

    // Locate an ECM stream with a currently valid pair of CW
    ECMStreamPtr pecm;
    for (std::set<PID>::const_iterator it = ss.ecm_pids.begin(); pecm.isNull() && it != ss.ecm_pids.end(); ++it) {
        pecm = getOrCreateECMStream (*it);
        if (!pecm->cw_valid) {
            pecm.clear();
        }
    }
    if (pecm.isNull()) {
        // No ECM stream has valid Control Word now, cannot descramble
        return TSP_OK;
    }

    // We found a valid CW, check if new CW were deciphered
    if ((scv == SC_EVEN_KEY && pecm->new_cw_even) || (scv == SC_ODD_KEY && pecm->new_cw_odd)) {

        // A new CW was deciphered. Convert it into a DVB-CSA key context.
        // In asynchronous mode, the CW are accessed under mutex protection.

        if (!_synchronous) {
            _mutex.acquire();
        }

        if (_aes128_dvs042) {
            if (!pecm->dvs042.setIV (_iv.data(), _iv.size())) {
                tsp->error ("error setting initialization vector in AES-128/DVS042 engine");
                _abort = true;
                return TSP_END;
            }
            uint8_t key[2 * CW_BYTES];
            ::memcpy(key, pecm->cw_even, CW_BYTES);            // Flawfinder: ignore: memcpy()
            ::memcpy(key + CW_BYTES, pecm->cw_odd, CW_BYTES);  // Flawfinder: ignore: memcpy()
            if (!pecm->dvs042.setKey (key, 2 * CW_BYTES)) {
                tsp->error ("error setting descrambling key in AES-128/DVS042 engine");
                _abort = true;
                return TSP_END;
            }
            pecm->new_cw_even = false;
            pecm->new_cw_odd = false;
        }
        else if (scv == SC_EVEN_KEY) {
            pecm->key_even.init(pecm->cw_even, _cw_mode);
            pecm->new_cw_even = false;
        }
        else {
            pecm->key_odd.init(pecm->cw_odd, _cw_mode);
            pecm->new_cw_odd = false;
        }

        if (!_synchronous) {
            _mutex.release();
        }
    }

    // Descramble the packet payload
    uint8_t* const pl = pkt.getPayload();
    size_t const pl_size = pkt.getPayloadSize();
    if (_aes128_dvs042) {
        uint8_t tmp[PKT_SIZE];
        assert (pl_size < sizeof(tmp));
        if (!pecm->dvs042.decrypt (pl, pl_size, tmp, pl_size)) {
            tsp->error ("AES decrypt error");
            return TSP_END;
        }
        ::memcpy (pl, tmp, pl_size);  // Flawfinder: ignore: memcpy()
    }
    else {
        Scrambling& scr (scv == SC_EVEN_KEY ? pecm->key_even : pecm->key_odd);
        scr.decrypt (pl, pl_size);

        // Trace CW change in PIDs
        if (scv != ss.last_scv) {
            ss.last_scv = scv;
            uint8_t cw[CW_BYTES];
            const bool get_cw_ok = scr.getCW (cw, sizeof(cw));
            assert (get_cw_ok);
            tsp->debug ("packet %" FMT_INT64 "d, PID %d (0x%04X), new CW (%s): %02X %02X %02X %02X %02X %02X %02X %02X",
                        _packet_count - 1, int (pid), int (pid),
                        scv == SC_EVEN_KEY ? "even" : "odd",
                        int (cw[0]), int (cw[1]), int (cw[2]), int (cw[3]),
                        int (cw[4]), int (cw[5]), int (cw[6]), int (cw[7]));
        }
    }

    // Reset scrambling_control_value to zero in TS header
    pkt.setScrambling (SC_CLEAR);

    return TSP_OK;
}
