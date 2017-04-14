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

#pragma once
#include "tsPlugin.h"
#include "tsSafePtr.h"
#include "tsService.h"
#include "tsScrambling.h"
#include "tsSectionDemux.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsMemoryUtils.h"
#include "tsAES.h"
#include "tsDVS042.h"

namespace ts {

    class TSDUCKDLL AbstractDescrambler:
        public ProcessorPlugin,
        protected TableHandlerInterface,
        private Thread
    {
    public:
        // Implementation of plugin API.
        // If overridden by concrete descrambler,
        // superclass must be explicitely invoked.
        AbstractDescrambler (TSP* tsp_,
                             const std::string& description_ = "",
                             const std::string& syntax_ = "",
                             const std::string& help_ = "");
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    protected:
        // Specify to use DVB-CSA descrambling (the default).
        // Must be invoked before startDescrambler()
        void useDVBCSA () {_aes128_dvs042 = false;}

        // Specify to use AES-128 in DVS042 (single key) mode instead of DVB-CSA
        // Must be invoked before startDescrambler()
        void useAES128DVS042 () {_aes128_dvs042 = true;}

        // Set initialization vector for chained modes (not DVB-CSA)
        // Must be invoked before startDescrambler()
        void setIV (const ByteBlock& iv) {_iv = iv;}

        // Start abstract descrambler. Should be invoked from start().
        bool startDescrambler (bool           synchronous,      // Synchronous ECM deciphering
                               bool           reduce_entropy,   // Perform entropy reduction on CW
                               const Service& service,          // Service to descramble (by name, id or none)
                               size_t         stack_usage = 0); // Stack usage for ECM deciphering (O for default)

        // Check a CA_descriptor from a PMT, must be implemented by concrete descramblers.
        // If the descrambler can manage the ECM from this PID, return true.
        // The private part of CA_descriptor is passed as priv / priv_size.
        virtual bool checkCADescriptor (uint16_t cas_id, const uint8_t* priv, size_t priv_size) = 0;

        // Check if the descrambler may decipher an ECM, must be implemented by concrete descramblers
        // The ecm buffer contains the CMT section payload, without section header.
        // Return false if this ECM cannot be deciphered.
        // Return true if it may be deciphered, altough it may fail when actually submitted.
        virtual bool checkECM (const uint8_t* ecm, size_t ecm_size) = 0;

        // Decipher an ECM, return the two control words, must be implemented by concrete descramblers
        // The ecm buffer contains the CMT section payload, without section header.
        // The cw_even and cw_odd buffer size must be at least ts::CW_BYTES.
        // Return true on success, false on error.
        virtual bool decipherECM (const uint8_t* ecm, size_t ecm_size, uint8_t* cw_even, uint8_t* cw_odd) = 0;

        // Invoked by the demux when a complete table is available.
        // If overridden by concrete descrambler,
        // superclass must be explicitely invoked.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

    private:
        struct ScrambledStream;
        struct ECMStream;
        typedef SafePtr <ECMStream, NullMutex> ECMStreamPtr;
        typedef std::map <PID, ScrambledStream> ScrambledStreamMap;
        typedef std::map <PID, ECMStreamPtr> ECMStreamMap;

        // Abstract descrambler private data
        Scrambling::EntropyMode _cw_mode;
        PacketCounter      _packet_count;      // Packet counter in TS
        bool               _abort;             // Error, abort asap
        bool               _synchronous;       // Synchronous ECM deciphering
        bool               _aes128_dvs042;     // Use AES-128 in DVS 042 mode instead of DVB-CSA
        ByteBlock          _iv;                // Initialization vector if chained mode (not DVB-CSA)
        Service            _service;           // Service to descramble (by name, id or none)
        size_t             _stack_usage;       // Stack usage for ECM deciphering
        SectionDemux       _demux;             // Section demux
        ECMStreamMap       _ecm_streams;       // ECM streams, indexed by PID
        ScrambledStreamMap _scrambled_streams; // ECM streams, indexed by PID
        Mutex              _mutex;             // Exclusive access to protected areas
        Condition          _ecm_to_do;         // Notify thread to process ECM
        // -- start of protected area --
        bool               _stop_thread;       // Terminate ECM processing thread

        // Description of a scrambled stream
        struct ScrambledStream
        {
            std::set<PID> ecm_pids;  // PIDs of ECM streams
            uint8_t         last_scv;  // Last scrambling control value on this PID

            // Constructor
            ScrambledStream() : ecm_pids(), last_scv (SC_CLEAR) {}
        };

        // Description of an ECM stream
        struct ECMStream
        {
            TID         last_tid;             // Last table id (0x80 or 0x81)
            Scrambling  key_even;             // DVB-CSA preprocessed CW (even)
            Scrambling  key_odd;              // DVB-CSA preprocessed CW (odd)
            DVS042<AES> dvs042;               // AES cipher in DVS 042 mode (not DVB-CSA)
            // -- start of write-protected, read-volative area --
            volatile bool cw_valid;           // CW's are valid
            volatile bool new_cw_even;        // New CW available (even)
            volatile bool new_cw_odd;         // New CW available (odd)
            // -- start of protected area --
            bool   new_ecm;                   // New ECM available
            size_t ecm_size;                  // Used size in ECM
            uint8_t  ecm[MAX_PSI_SECTION_SIZE]; // Last received ECM
            uint8_t  cw_even[CW_BYTES];         // Last valid CW (even)
            uint8_t  cw_odd[CW_BYTES];          // Last valid CW (odd)

            // Constructor:
            ECMStream() :
                last_tid (TID_NULL),
                key_even (),
                key_odd (),
                dvs042 (),
                cw_valid (false),
                new_cw_even (false),
                new_cw_odd (false),
                new_ecm (false),
                ecm_size (0)
            {
                TS_ZERO (ecm);
                TS_ZERO (cw_even);
                TS_ZERO (cw_odd);
            }
        };

        // Get the ECM stream for a PID, create it if non existent
        ECMStreamPtr getOrCreateECMStream (PID);

        // Process one ECM (the one in ECMStream::ecm).
        // In asynchronous mode, this method must be invoked with the mutex held. The method
        // releases the mutex while deciphering the ECM and relocks it before exiting.
        void processECM (ECMStream&);

        // Analyze a list of descriptors, looking for ECM PID's
        void analyzeCADescriptors (const DescriptorList& dlist, std::set<PID>& ecm_pids);

        // ECM deciphering thread
        virtual void main();

        // Process specific tables
        void processPAT (const PAT&);
        void processPMT (const PMT&);
        void processSDT (const SDT&);
        void processCMT (const Section&);
    };
}
