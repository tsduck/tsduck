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
//!
//!  @file
//!  A class which analyzes a complete transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsPESDemux.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsTime.h"
#include "tsSafePtr.h"

namespace ts {

    class TSDUCKDLL TSAnalyzer:
        private TableHandlerInterface,
        private SectionHandlerInterface,
        private PESHandlerInterface
    {
    public:
        // Constructor & destructor
        TSAnalyzer (BitRate bitrate_hint = 0);
        ~TSAnalyzer ();
 
        // The following method feeds the analyzer with a TS packet.
        void feedPacket (const TSPacket&);

        // Reset the analysis context.
        void reset ();

        // Specify a "bitrate hint" for the analysis. It is the user-specified
        // bitrate in bits/seconds, based on 188-byte packets. The bitrate is
        // optional: if specified as zero, the analysis is based on the PCR values.
        void setBitrateHint (BitRate = 0);

        // Set the number of consecutive packet errors after which a packet is
        // considered as suspect if it does not belong to a previously known PID.
        // If set to zero, suspect packet detection is disabled.
        // Initially set to the default value 1.
        void setMinErrorCountBeforeSuspect (uint64_t count) {_min_error_before_suspect = count;}

        // Set the maximum number of consecutive suspect packets.
        // When that number of consecutive suspect packets is reached, the next
        // packet will not be considered for suspect detection.
        // Initially set to the default value 1.
        void setMaxConsecutiveSuspectCount (uint64_t count) {_max_consecutive_suspects = count;}

        // Return the list of service ids
        void getServiceIds (std::vector<uint16_t>&);

        // Return the list of PIDs
        void getPIDs (std::vector<PID>&);

        // Return the list of global PIDs
        void getGlobalPIDs (std::vector<PID>&);

        // Return the list of unreferenced PIDs
        void getUnreferencedPIDs (std::vector<PID>&);

        // Return the list of PIDs for one service id
        void getPIDsOfService (std::vector<PID>&, uint16_t);

        // Return the list of PIDs carrying PES packets
        void getPIDsWithPES (std::vector<PID>&);

    protected:

        // -------------------
        // Service description
        // -------------------

        // This protected inner class contains the analysis context for one service.
        class TSDUCKDLL ServiceContext
        {
        public:
            // Public members - Synthetic data (do not modify outside ServiceContext methods)
            const uint16_t service_id;         // Service id
            uint16_t       orig_netw_id;       // Original network id
            uint8_t        service_type;       // Service type
            std::string  name;               // Service name
            std::string  provider;           // Service provider name
            PID          pmt_pid;            // PID of PMT
            PID          pcr_pid;            // PID of PCR's (if any)
            size_t       pid_cnt;            // # PID's
            size_t       scrambled_pid_cnt;  // # scrambled PID's
            uint64_t       ts_pkt_cnt;         // # TS packets
            uint32_t       bitrate;            // Average service bitrate in b/s
            bool         carry_ssu;          // Carry System Software Update

            // Constructor / destructor
            ServiceContext (uint16_t serv_id);
            ~ServiceContext ();

            // Return a displayable service or provider name
            std::string getName() const;
            std::string getProvider() const;

        private:
            // Unreachable constructor:
            ServiceContext ();
        };
        typedef SafePtr <ServiceContext, NullMutex> ServiceContextPtr;

        // Map of ServiceContext, indexed by service id
        typedef std::map <uint16_t, ServiceContextPtr> ServiceContextMap;

        // Set of service ids
        typedef std::set <uint16_t> ServiceIdSet;

    protected:

        // -------------------
        // Section description
        // -------------------

        // This protected inner class contains the analysis context for one TID/TIDext into one PID.
        class TSDUCKDLL ETIDContext
        {
        public:
            // Public members - Synthetic data (do not modify outside ETIDContext methods)
            const ETID etid;                      // ETID value
            uint64_t     table_count;               // Number of occurences of this table (section# 0)
            uint64_t     section_count;             // Number of occurences of sections in this table
            uint64_t     repetition_ts;             // Average # TS packets between occurences of this table (section# 0)
            uint64_t     min_repetition_ts;         // Minimum # TS packets between occurences of this table (section# 0)
            uint64_t     max_repetition_ts;         // Maximum # TS packets between occurences of this table (section# 0)
            uint8_t      first_version;             // First version encountered
            uint8_t      last_version;              // First version encountered
            std::bitset <SVERSION_MAX> versions;  // Set of versions

            // Public members - Analysis data: Repetition interval evaluation:
            uint64_t     first_pkt;                 // Last packet index of first section# 0
            uint64_t     last_pkt;                  // Last packet index of last section# 0

            // Constructor / destructor
            ETIDContext (const ETID&);
            ~ETIDContext ();
        };
        typedef SafePtr <ETIDContext, NullMutex> ETIDContextPtr;

        // Map of ETIDContext, indexed by ETID
        typedef std::map <ETID, ETIDContextPtr> ETIDContextMap;

    protected:

        // -------------------
        // PID description
        // -------------------

        // This protected inner class contains the analysis context for one PID.
        class TSDUCKDLL PIDContext
        {
        public:
            // Public members - Synthetic data (do not modify outside PIDContext methods)
            const PID    pid;             // PID value
            std::string  description;     // Readable description string (ie. "MPEG-2 Audio")
            std::string  comment;         // Additional description (ie. language)
            StringVector attributes;      // Audio or video attributes (several lines if attributes changed)
            ServiceIdSet services;        // List of service ids the PID belongs to
            bool         is_pmt_pid;      // Is the PMT PID for this service
            bool         is_pcr_pid;      // Is the PCR PID for this service
            bool         referenced;      // Is referenced (by service or global)
            bool         optional;        // Optional PID, don't display report if no packet
            bool         carry_pes;       // This PID carries PES packets
            bool         carry_section;   // This PID carries sections
            bool         carry_ecm;       // This PID carries ECM's
            bool         carry_emm;       // This PID carries EMM's
            bool         carry_audio;     // This PID carries audio data
            bool         carry_video;     // This PID carries video data
            bool         scrambled;       // Contains some scrambled packets
            bool         same_stream_id;  // All PES packets have same stream_id
            uint8_t      pes_stream_id;   // Stream_id in PES packets on this PID
            uint64_t     ts_pkt_cnt;      // # TS packets
            uint64_t     ts_af_cnt;       // # TS packets with adaptation field
            uint64_t     unit_start_cnt;  // # unit_start in packets
            uint64_t     pl_start_cnt;    // # unit_start & has_payload in packets
            uint64_t     pmt_cnt;         // # PMT (for PMT PID's)
            uint64_t     crypto_period;   // average # TS packets per crypto-period
            uint64_t     unexp_discont;   // # unexpected discontinuities
            uint64_t     exp_discont;     // # expected discontinuities
            uint64_t     duplicated;      // # duplicated packets
            uint64_t     ts_sc_cnt;       // # scrambled packets
            uint64_t     inv_ts_sc_cnt;   // # invalid scrambling control in TS headers
            uint64_t     inv_pes_start;   // # invalid PES start code
            uint64_t     pcr_cnt;         // # PCR's
            uint32_t     ts_pcr_bitrate;  // Average TS bitrate in b/s (eval from PCR)
            uint32_t     bitrate;         // Average PID bitrate in b/s
            std::string  language;        // For audio or subtitles (3 chars)
            uint16_t     cas_id;          // For EMM and ECM streams
            uint32_t     cas_operator;    // For EMM and ECM streams, when applicable
            ETIDContextMap sections;      // List of sections in this PID
            std::set<uint32_t> ssu_oui;     // Set of applicable OUI's for SSU

            // Public members - Analysis data:
            uint8_t        cur_continuity;  // Current continuity count
            // Public members - Analysis data: Crypto-period evaluation:
            uint8_t        cur_ts_sc;       // Current scrambling control in TS header
            uint64_t       cur_ts_sc_pkt;   // First packet index of current crypto-period
            uint64_t       cryptop_cnt;     // # crypto-periods
            uint64_t       cryptop_ts_cnt;  // # TS packets in all crypto-periods
            // Public members - Analysis data: Bitrate evaluation
            uint64_t       last_pcr;        // Last PCR value
            uint64_t       last_pcr_pkt;    // Index of packet with last PCR
            uint64_t       ts_bitrate_sum;  // Sum of all computed TS bitrates
            uint64_t       ts_bitrate_cnt;  // # computed TS bitrates

            // Constructor / destructor
            PIDContext (PID pid, const std::string& description = "Unreferenced");
            ~PIDContext ();

            // Register a service id for the PID
            void addService (uint16_t service_id);

            // Return a full description, with comment and optionally attributes
            std::string fullDescription (bool include_attributes) const;

        private:
            // Unreachable constructor:
            PIDContext();
        };
        typedef SafePtr <PIDContext, NullMutex> PIDContextPtr;

        // Map of PIDContext, indexed by PID
        typedef std::map <PID, PIDContextPtr> PIDContextMap;

    protected:

        // ----------------------------
        // Transport stream description
        // ----------------------------

        // Update the global statistics value if internal data were modified.
        void recomputeStatistics();

        // TSAnalyzer protected members.
        // Accessible to subclasses, valid after calling recomputeStatistics().
        // Important: subclasses shall not modify these fields, just read them.
        uint16_t    _ts_id;              // Transport stream id
        bool        _ts_id_valid;        // Transport stream id is valid
        uint64_t    _ts_pkt_cnt;         // # TS packets
        uint64_t    _invalid_sync;       // # packets with invalid sync byte (not 0x47)
        uint64_t    _transport_errors;   // # packets with transport error
        uint64_t    _suspect_ignored;    // # suspect packets, ignored
        size_t      _pid_cnt;            // # PID's (with actual packets)
        size_t      _scrambled_pid_cnt;  // # scrambled PID's
        size_t      _pcr_pid_cnt;        // # PID's with PCR's
        size_t      _global_pid_cnt;     // # global PID's (ref but no service)
        size_t      _global_scr_pids;    // # scrambled global PID's
        uint64_t    _global_pkt_cnt;     // # packets in global PID's
        uint32_t    _global_bitrate;     // Bitrate for global PID's
        size_t      _unref_pid_cnt;      // # unreferenced PID's
        size_t      _unref_scr_pids;     // # scrambled unreferenced PID's
        uint64_t    _unref_pkt_cnt;      // # packets in unreferenced PID's
        uint32_t    _unref_bitrate;      // Bitrate for unreferenced PID's
        uint32_t    _ts_pcr_bitrate_188; // Average TS bitrate in b/s (eval from PCR)
        uint32_t    _ts_pcr_bitrate_204; // Average TS bitrate in b/s (eval from PCR)
        uint32_t    _ts_user_bitrate;    // User-specified TS bitrate (if any)
        uint32_t    _ts_bitrate;         // TS bitrate (either from PCR or options)
        MilliSecond _duration;           // Total broadcast duration
        Time        _first_utc;          // First system UTC time (first packet)
        Time        _last_utc;           // Last system UTC time (recomputeStatistics)
        Time        _first_local;        // First system local time (first packet)
        Time        _last_local;         // Last system local time (recomputeStatistics)
        Time        _first_tdt;          // First TDT UTC time stamp
        Time        _last_tdt;           // Last TDT UTC time stamp
        Time        _first_tot;          // First TOT local time stamp
        Time        _last_tot;           // Last TOT local time stamp
        std::string _country_code;       // TOT country code
        uint16_t    _scrambled_services_cnt; // # scrambled services;
        std::bitset <TID_MAX> _tid_present;  // Array of detected tables
        PIDContextMap     _pids;        // Description of PIDs
        ServiceContextMap _services;    // Description of services, map key: service id.

    private:
        // Check if a PID context exists.
        bool pidExists (PID pid) const {return _pids.find (pid) != _pids.end();}

        // Return a PID context. Allocate a new entry if PID not found.
        PIDContextPtr getPID (PID pid, const std::string& description = "Unreferenced");

        // Return an ETID context. Allocate a new entry if ETID not found.
        ETIDContextPtr getETID (const Section&);

        // Return a service context. Allocate a new entry if service not found.
        ServiceContextPtr getService (uint16_t service_id);

        // Analyze the various PSI tables
        void analyzePAT (const PAT&);
        void analyzeCAT (const CAT&);
        void analyzePMT (PID pid, const PMT&);
        void analyzeSDT (const SDT&);
        void analyzeTDT (const TDT&);
        void analyzeTOT (const TOT&);

        // Analyse a list of descriptors.
        // If svp is not 0, we are in the PMT of the specified service.
        // If ps is not 0, we are in the description of this PID in a PMT.
        void analyzeDescriptors (const DescriptorList& descs, ServiceContext* svp = 0, PIDContext* ps = 0);

        // Analyse one CA descriptor, either from the CAT or a PMT.
        // If svp is not 0, we are in the PMT of the specified service.
        // If ps is not 0, we are in the description of this PID in a PMT.
        // If svp is 0, we are in the CAT.
        void analyzeCADescriptor (const Descriptor& desc, ServiceContext* svp = 0, PIDContext* ps = 0);

        // Implementation of TableHandlerInterface
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Implementation of SectionHandlerInterface
        virtual void handleSection (SectionDemux&, const Section&);

        // Implementation of PESHandlerInterface
        virtual void handleNewAudioAttributes (PESDemux&, const PESPacket&, const AudioAttributes&);
        virtual void handleNewVideoAttributes (PESDemux&, const PESPacket&, const VideoAttributes&);
        virtual void handleNewAVCAttributes (PESDemux&, const PESPacket&, const AVCAttributes&);
        virtual void handleNewAC3Attributes (PESDemux&, const PESPacket&, const AC3Attributes&);

        // TSAnalyzer private members (state data, used during analysis):
        bool         _modified;                  // Internal data modified, need recomputeStatistics
        uint64_t     _ts_bitrate_sum;            // Sum of all computed TS bitrates
        uint64_t     _ts_bitrate_cnt;            // # computed TS bitrates
        uint64_t     _preceding_errors;          // # contiguous invalid packets before current packet
        uint64_t     _preceding_suspects;        // # contiguous suspects packets before current packet
        uint64_t     _min_error_before_suspect;  // required # invalid packets before starting suspect
        uint64_t     _max_consecutive_suspects;  // max # consecutive suspect packets before clearing suspect
        SectionDemux _demux;                     // PSI tables analysis
        PESDemux     _pes_demux;                 // Audio/video analysis
    };
}
