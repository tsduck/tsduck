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
//!  Multiplexer (tsmux) core engine.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"
#include "tsMuxerArgs.h"
#include "tstsmuxInputExecutor.h"
#include "tstsmuxOutputExecutor.h"
#include "tsTime.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsPCRMerger.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsNIT.h"

namespace ts {
    namespace tsmux {
        //!
        //! Multiplexer (tsmux) core engine.
        //! @ingroup plugin
        //!
        class Core: private Thread, private SectionProviderInterface
        {
            TS_NOBUILD_NOCOPY(Core);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of plugin event handlers.
            //! @param [in,out] log Log report.
            //!
            Core(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log);

            //!
            //! Destructor.
            //!
            virtual ~Core() override;

            //!
            //! Start the @c tsmux processing.
            //! @return True on success, false on error.
            //!
            bool start();

            //!
            //! Stop the @c tsmux processing.
            //!
            void stop();

            //!
            //! Wait for completion of all plugin threads.
            //!
            void waitForTermination();

        private:
            // Description of an input stream.
            class Input;

            // Description of the origin of a PID or service.
            class Origin
            {
            public:
                size_t plugin_index;
                bool   conflict_detected;
                Origin(size_t index = NPOS) : plugin_index(index), conflict_detected(false) {}
            };

            // Reference clock of a PID in the output stream.
            class PIDClock
            {
            public:
                uint64_t      pcr_value;   // Last PCR value in this PID.
                PacketCounter pcr_packet;  // Packet index in output stream of last PCR.
                PIDClock(uint64_t value = INVALID_PCR, PacketCounter packet = 0) : pcr_value(value), pcr_packet(packet) {}
            };

            // Core private members.
            const PluginEventHandlerRegistry& _handlers;
            Report&             _log;               // Asynchronous log report.
            const MuxerArgs&    _opt;               // Command line options.
            DuckContext         _duck;              // TSDuck execution context.
            volatile bool       _terminate;         // Termination request.
            BitRate             _bitrate;           // Constant output bitrate.
            PacketCounter       _output_packets;    // Count of output packets which were sent.
            size_t              _time_input_index;  // Input plugin index containing time reference (TDT/TOT).
            std::vector<Input*> _inputs;            // Input plugins threads.
            OutputExecutor      _output;            // Output plugin thread.
            std::set<size_t>    _terminated_inputs; // Set of terminated input plugins.
            CyclingPacketizer   _pat_pzer;          // Packetizer for output PAT.
            CyclingPacketizer   _cat_pzer;          // Packetizer for output CAT.
            CyclingPacketizer   _nit_pzer;          // Packetizer for output NIT's.
            CyclingPacketizer   _sdt_bat_pzer;      // Packetizer for output SDT/BAT.
            Packetizer          _eit_pzer;          // Packetizer for output EIT's.
            PAT                 _output_pat;        // PAT for output stream.
            CAT                 _output_cat;        // CAT for output stream.
            SDT                 _output_sdt;        // SDT Actual for output stream.
            NIT                 _output_nit;        // NIT Actual for output stream.
            size_t              _max_eits;          // Maximum number of buffered EIT sections.
            std::list<SectionPtr>     _eits;            // List of EIT sections to insert.
            std::map<PID,Origin>      _pid_origin;      // Map of PID's to original input stream.
            std::map<uint16_t,Origin> _service_origin;  // Map of service ids to original input stream.

            // Implementation of Thread.
            virtual void main() override;

            // Get a packet from plugin at given index. If none is available, try next input and so on.
            // Update the plugin index. Return false if all input plugins were tried without success.
            bool getInputPacket(size_t& input_index, TSPacket& pkt, TSPacketMetadata& pkt_data);

            // Try to extract a UTC time from a TDT or TOT in one TS packet.
            bool getUTC(Time& utc, const TSPacket& pkt);

            // Implementation of SectionProviderInterface (for output EIT provision).
            virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
            virtual bool doStuffing() override;

            //----------------------------------------------------------------
            // Description of an input stream.
            //----------------------------------------------------------------

            class Input : private TableHandlerInterface, SectionHandlerInterface
            {
                TS_NOBUILD_NOCOPY(Input);
            public:
                // Constructor.
                Input(Core& core, size_t index);

                // Initialize the plugin.
                bool init() { return _input.plugin()->getOptions() && _input.plugin()->start(); }

                // Uninitialize the plugin. Can be done when the executor was not started only.
                bool uninit() { return _input.plugin()->stop(); }

                // Start the executor thread.
                bool start() { return _input.start(); }

                // Request the executor thread to terminate.
                void terminate() { _input.terminate(); _terminated = true; }

                // Check if the input is terminated (or terminating).
                bool isTerminated() const { return _terminated; }

                // Wait for the executor thread to terminate.
                void waitForTermination() { _input.waitForTermination(); }

                // Get one input packet. Return false when none is immediately available.
                bool getPacket(TSPacket& pkt, TSPacketMetadata& pkt_data);

            private:
                Core&            _core;           // Reference to the parent Core.
                const size_t     _plugin_index;   // Input plugin index.
                bool             _terminated;     // Detected that the executor thread has terminated.
                bool             _got_ts_id;      // Input transport stream id is known.
                uint16_t         _ts_id;          // Input transport stream id (when _got_ts_id is true).
                InputExecutor    _input;          // Input plugin thread.
                SectionDemux     _demux;          // Demux for PSI/SI (except PMT's and EIT's).
                SectionDemux     _eit_demux;      // Demux for EIT's.
                PCRMerger        _pcr_merger;     // Adjust PCR in input packets to be synchronized with the output stream.
                NIT              _nit;            // NIT waiting to be merged.
                PacketCounter    _next_insertion; // Insertion point of next packet.
                TSPacket         _next_packet;    // Next packet to insert if already received but not yet inserted.
                TSPacketMetadata _next_metadata;  // Associated metadata.
                std::map<PID,PIDClock> _pid_clocks;  // Output clock of each input PID.

                // Adjust the PCR of a packet before insertion.
                void adjustPCR(TSPacket& pkt);

                // Receive a PSI/SI table.
                virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;
                void handlePAT(const PAT&);
                void handleCAT(const CAT&);
                void handleNIT(const NIT&);
                void handleSDT(const SDT&);

                // Receive an EIT section.
                virtual void handleSection(SectionDemux& demux, const Section& section) override;
            };
        };
    }
}
