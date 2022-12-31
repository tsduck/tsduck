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
//!
//!  @file
//!  Abstract base class for DVB descrambler plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSafePtr.h"
#include "tsSection.h"
#include "tsServiceDiscovery.h"
#include "tsTSScrambling.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"
#include "tsMemory.h"

namespace ts {

    //!
    //! Abstract base class for DVB descrambler plugins.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractDescrambler:
        public ProcessorPlugin,
        protected SignalizationHandlerInterface,
        protected SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(AbstractDescrambler);
    public:
        // Implementation of ProcessorPlugin interface.
        // If overridden by descrambler subclass, superclass must be explicitly invoked.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    protected:
        //!
        //! Default stack usage allocated to CAS-specific processing of an ECM.
        //! @see decipherECM()
        //!
        static const size_t DEFAULT_ECM_THREAD_STACK_USAGE = 128 * 1024;

        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Object to communicate with the Transport Stream Processor main executable.
        //! @param [in] description A short one-line description, eg. "Descrambler for 'xyz' CAS".
        //! @param [in] syntax A short one-line syntax summary, default: u"[options] [service]".
        //! @param [in] stack_usage Stack usage for asynchronous ECM deciphering.
        //!
        AbstractDescrambler(TSP*           tsp,
                            const UString& description = UString(),
                            const UString& syntax = u"[options] [service]",
                            size_t         stack_usage = DEFAULT_ECM_THREAD_STACK_USAGE);

        //!
        //! Check a CA_descriptor from a PMT.
        //!
        //! Must be implemented by subclasses (concrete descramblers).
        //!
        //! This method is invoked by the superclass when a CA_descriptor is found in a PMT.
        //!
        //! The subclass must check if it can descramble ECM's from the corresponding PID.
        //! This method shall check two things:
        //! - Verify that the CA_system_id is compatible with the concrete descrambler.
        //! - Optionally verify that the private part of the CA_descriptor is
        //!   compatible with, for instance, the capabilities of the smartcard.
        //!
        //! Example: A CAS may set an operator id in the private part of the
        //! CA_descriptor and the smartcard may have returned the list of operators
        //! during initialization. If the smartcard cannot manage any operator in
        //! the CA_descriptor, this ECM stream is probably for another operator and
        //! the descrambler should ignore it.
        //!
        //! @param [in] cas_id CA_system_id value.
        //! @param [in] priv Private part of the CA_descriptor.
        //! @return True if the descrambler can manage the ECM from this PID.
        //!
        virtual bool checkCADescriptor(uint16_t cas_id, const ByteBlock& priv) = 0;

        //!
        //! Check if the descrambler may decipher an ECM.
        //!
        //! Must be implemented by subclasses (concrete descramblers).
        //!
        //! This method is invoked when a new ECM is received from a valid ECM stream
        //! which was validated by checkCADescriptor(). This method may perform
        //! additional checks on the ECM itself. But this method shall not attempt to
        //! decipher the ECM or submit it to a smartcard or perform any time-consuming
        //! processing.
        //!
        //! @param [in] ecm CMT section (typically an ECM).
        //! @return False if this ECM cannot be deciphered.
        //! True if it may be deciphered, although it may fail when actually submitted.
        //! @see decipherECM()
        //! @see checkCADescriptor()
        //!
        virtual bool checkECM(const Section& ecm) = 0;

        //!
        //! Description of a control word.
        //!
        class CWData
        {
        public:
            uint8_t   scrambling;  //!< Scrambling mode, as defined in scrambling_descriptor.
            ByteBlock cw;          //!< Control word, typically 8 or 16 bytes.
            ByteBlock iv;          //!< Initialization vector, typically empty or 16 bytes.

            //!
            //! Constructor.
            //! @param [in] mode Scrambling mode.
            //!
            CWData(uint8_t mode = SCRAMBLING_DVB_CSA2);
        };

        //!
        //! Decipher an ECM, return up to two control words, even and/or odd.
        //!
        //! Must be implemented by subclasses (concrete descramblers).
        //!
        //! By default (without -\-synchronous option), this method is executed in the
        //! context of a separate thread. It may take any necessary time to process
        //! an ECM, including submitting it to a smartcard. This method shall return
        //! either an odd CW, even CW or both. Missing CW's shall be empty.
        //!
        //! @param [in] ecm CMT section (typically an ECM).
        //! @param [in,out] cw_even Returned even CW. Empty if the ECM contains no even CW.
        //! On input, the scrambling field is set to the current descrambling mode.
        //! It output, the scrambling field can be updated if the ECM specifies a new one.
        //! @param [in,out] cw_odd Returned odd CW. Empty if the ECM contains no odd CW.
        //! On input, the scrambling field is set to the current descrambling mode.
        //! It output, the scrambling field can be updated if the ECM specifies a new one.
        //! @return True on success, false on error. Missing ECM's (odd or even) in the ECM
        //! shall not be considered as errors. Unable to decipher the ECM is an error.
        //!
        virtual bool decipherECM(const Section& ecm, CWData& cw_even, CWData& cw_odd) = 0;

    protected:
        //!
        //! This hook is invoked when a new PMT is available.
        //! Implementation of SignalizationHandlerInterface.
        //! If overridden by a concrete descrambler, the superclass must be explicitly invoked.
        //! @param [in] table A reference to the new PMT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handlePMT(const PMT& table, PID pid) override;

        //!
        //! This hook is invoked when a complete section is available.
        //! Implementation of SectionHandlerInterface.
        //! If overridden by a concrete descrambler, the superclass must be explicitly invoked.
        //! @param [in,out] demux The demux which sends the section.
        //! @param [in] section The new section from the demux.
        //!
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

    private:
        // Description of a scrambled stream with its possible ECM PID's.
        // Each elementary stream in the service can be potentially scrambled.
        // Each of them has an entry in _scrambled_streams with the list of valid ECM PID's.
        // Here, a stream may have several potential ECM PID's. Although only one ECM stream
        // is normally used for a given descrambler configuration, we may not know which one
        // to use from the beginning. If the CAS in the subclass is very selective, checkCADescriptor()
        // will indicate the precise ECM PID to use. Otherwise, we may have more than one candidate.
        // We filter ECM's on all these PID's and we hope that the subclass will indicate which
        // ECM's are the right ones in checkECM(). At worst, we try to decipher all ECM's from
        // all ECM streams and decipherECM() will fail with ECM's we cannot handle.
        class ScrambledStream
        {
        public:
            // Constructor
            ScrambledStream() : ecm_pids() {}

            std::set<PID> ecm_pids;  // PIDs of ECM streams
        };

        // Map of scrambled streams in the service, indexed by PID.
        typedef std::map<PID, ScrambledStream> ScrambledStreamMap;

        // Description of an ECM stream
        class ECMStream
        {
            TS_NOBUILD_NOCOPY(ECMStream);
        public:
            // Constructor
            ECMStream(AbstractDescrambler* parent);

            TID           last_tid;     // Last table id (0x80 or 0x81)
            TSScrambling  scrambling;   // Descrambling using CW from the ECM's of this stream.
            // -- start of write-protected, read-volatile area --
            volatile bool cw_valid;     // CW's are valid
            volatile bool new_cw_even;  // New CW available (even)
            volatile bool new_cw_odd;   // New CW available (odd)
            // -- start of protected area --
            bool          new_ecm;      // New ECM available
            Section       ecm;          // Last received ECM
            CWData        cw_even;      // Last valid CW (even)
            CWData        cw_odd;       // Last valid CW (odd)
            // -- end of protected area --
        };

        typedef SafePtr<ECMStream, NullMutex> ECMStreamPtr;
        typedef std::map<PID, ECMStreamPtr> ECMStreamMap;

        // ECM deciphering thread
        class ECMThread : public Thread
        {
            TS_NOBUILD_NOCOPY(ECMThread);
        public:
            // Constructor.
            ECMThread(AbstractDescrambler* parent) : _parent(parent) {}

        private:
            // Thread entry point.
            virtual void main() override;

            // Link to parent descrambler.
            AbstractDescrambler* _parent;
        };

        // Get the ECM stream for a PID, create it if non existent
        ECMStreamPtr getOrCreateECMStream(PID);

        // Process one ECM (the one in ECMStream::ecm).
        // In asynchronous mode, this method must be invoked with the mutex held. The method
        // releases the mutex while deciphering the ECM and relocks it before exiting.
        void processECM(ECMStream&);

        // Analyze a list of descriptors from the PMT, looking for ECM PID's
        void analyzeDescriptors(const DescriptorList& dlist, std::set<PID>& ecm_pids, uint8_t& scrambling);

        // Abstract descrambler private data.
        bool               _use_service;       // Descramble a service (ie. not a specific list of PID's).
        bool               _need_ecm;          // We need to get control words from ECM's.
        bool               _abort;             // Error, abort asap.
        bool               _synchronous;       // Synchronous ECM deciphering.
        bool               _swap_cw;           // Swap even/odd CW from ECM.
        TSScrambling       _scrambling;        // Default descrambling (used with fixed control words).
        PIDSet             _pids;              // Explicit PID's to descramble.
        ServiceDiscovery   _service;           // Service to descramble (by name, id or none).
        size_t             _stack_usage;       // Stack usage for ECM deciphering.
        SectionDemux       _demux;             // Section demux to extract ECM's.
        ECMStreamMap       _ecm_streams;       // ECM streams, indexed by PID.
        ScrambledStreamMap _scrambled_streams; // Scrambled streams, indexed by PID.
        Mutex              _mutex;             // Exclusive access to protected areas
        Condition          _ecm_to_do;         // Notify thread to process ECM.
        ECMThread          _ecm_thread;        // Thread which deciphers ECM's.
        // -- start of protected area --
        bool               _stop_thread;       // Terminate ECM processing thread
        // -- end of protected area --
    };
}
