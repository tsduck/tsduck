//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class which analyzes a complete transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsPESDemux.h"
#include "tsT2MIDemux.h"
#include "tsLogicalChannelNumbers.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsNIT.h"
#include "tsSDT.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsMGT.h"
#include "tsVCT.h"
#include "tsSTT.h"
#include "tsTime.h"
#include "tsUString.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! A class which analyzes a complete transport stream.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSAnalyzer:
        private TableHandlerInterface,
        private SectionHandlerInterface,
        private InvalidSectionHandlerInterface,
        private PESHandlerInterface,
        private T2MIHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TSAnalyzer);
    public:
        //!
        //! Default constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the analyzer.
        //! @param [in] bitrate_hint Optional bitrate "hint" for the analysis. It is the user-specified
        //! bitrate in bits/seconds, based on 188-byte packets. The bitrate hint is optional:
        //! if specified as zero, the analysis is based on the PCR values.
        //! @param [in] bitrate_confidence Confidence level in @a bitrate_hint.
        //!
        explicit TSAnalyzer(DuckContext& duck, const BitRate& bitrate_hint = 0, BitRateConfidence bitrate_confidence = BitRateConfidence::LOW);

        //!
        //! Destructor.
        //!
        virtual ~TSAnalyzer() override;

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! The stream is analyzed by repeatedly calling feedPacket().
        //! @param [in] packet One TS packet from the stream.
        //!
        void feedPacket(const TSPacket& packet);

        //!
        //! Reset the analysis context.
        //!
        void reset();

        //!
        //! Specify a "bitrate hint" for the analysis.
        //! @param [in] bitrate_hint Optional bitrate "hint" for the analysis. It is the user-specified
        //! bitrate in bits/seconds, based on 188-byte packets. The bitrate hint is optional:
        //! if specified as zero, the analysis is based on the PCR values.
        //! @param [in] bitrate_confidence Confidence level in @a bitrate_hint.
        //!
        void setBitrateHint(const BitRate& bitrate_hint = 0, BitRateConfidence bitrate_confidence = BitRateConfidence::LOW);

        //!
        //! Set the number of consecutive packet errors threshold.
        //! @param [in] count The number of consecutive packet errors after which a packet is
        //! considered as suspect if it does not belong to a previously known PID.
        //! If set to zero, suspect packet detection is disabled.
        //! Initially set to the default value 1.
        //!
        void setMinErrorCountBeforeSuspect(uint64_t count)
        {
            _min_error_before_suspect = count;
        }

        //!
        //! Set the maximum number of consecutive suspect packets.
        //! @param [in] count When that number of consecutive suspect packets is reached,
        //! the next packet will not be considered for suspect detection.
        //! Initially set to the default value 1.
        //!
        void setMaxConsecutiveSuspectCount(uint64_t count)
        {
            _max_consecutive_suspects = count;
        }

        //!
        //! Get the list of service ids.
        //! @param [out] list The returned list of service ids.
        //!
        void getServiceIds(std::vector<uint16_t>& list);

        //!
        //! Get the list of all PID's.
        //! @param [out] list The returned list of all PID's.
        //!
        void getPIDs(std::vector<PID>& list);

        //!
        //! Get the list of global PID's.
        //! Global PID's are PID's which do not belong to a service.
        //! @param [out] list The returned list of global PID's.
        //!
        void getGlobalPIDs(std::vector<PID>& list);

        //!
        //! Get the list of unreferenced PID's.
        //! @param [out] list The returned list of unreferenced PID's.
        //!
        void getUnreferencedPIDs(std::vector<PID>& list);

        //!
        //! Get the list of PID's for one service id.
        //! @param [out] list The returned list of PID's.
        //! @param [in] service_id The service id.
        //!
        void getPIDsOfService(std::vector<PID>& list, uint16_t service_id);

        //!
        //! Get the list of PID's carrying PES packets.
        //! @param [out] list The returned list of PID's carrying PES packets.
        //!
        void getPIDsWithPES(std::vector<PID>& list);

    protected:

        // -------------------
        // Service description
        // -------------------

        //!
        //! This protected inner class contains the analysis context for one service.
        //!
        class TSDUCKDLL ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);
        public:
            // Public members - Synthetic data (do not modify outside ServiceContext methods)
            const uint16_t          service_id;       //!< Service id.
            std::optional<uint16_t> orig_netw_id {};  //!< Original network id.
            std::optional<uint16_t> lcn {};           //!< Logical channel number.
            uint8_t  service_type = 0;                //!< Service type.
            UString  name {};                         //!< Service name.
            UString  provider {};                     //!< Service provider name.
            PID      pmt_pid = 0;                     //!< PID of PMT.
            PID      pcr_pid = 0;                     //!< PID of PCR's (if any).
            size_t   pid_cnt = 0;                     //!< Number of PID's.
            size_t   scrambled_pid_cnt = 0;           //!< Number of scrambled PID's.
            uint64_t ts_pkt_cnt = 0;                  //!< Number of TS packets.
            BitRate  bitrate = 0;                     //!< Average service bitrate in b/s.
            bool     hidden = false;                  //!< Service is hidden from end-user.
            bool     carry_ssu = false;               //!< Carry System Software Update.
            bool     carry_t2mi = false;              //!< Carry T2-MI encasulated data.

            //!
            //! Constructor.
            //! @param [in] serv_id Service id.
            //!
            ServiceContext(uint16_t serv_id) : service_id(serv_id) {}

            //!
            //! Get a displayable service name.
            //! @return A displayable service name.
            //!
            UString getName() const;

            //!
            //! Get a displayable provider name.
            //! @return A displayable provider name.
            //!
            UString getProvider() const;

            //!
            //! Update service information from a descriptor list.
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in] descs Descriptor list from SDT or PMT.
            //!
            void update(DuckContext& duck, const DescriptorList& descs);
        };

        //!
        //! Safe pointer to a ServiceContext (not thread-safe).
        //!
        typedef SafePtr<ServiceContext, ts::null_mutex> ServiceContextPtr;

        //!
        //! Map of ServiceContext, indexed by service id.
        //!
        typedef std::map<uint16_t, ServiceContextPtr> ServiceContextMap;

        //!
        //! Set of service ids.
        //!
        typedef std::set<uint16_t> ServiceIdSet;

        //!
        //! Get a service context.
        //! Allocate a new entry if service is not found.
        //! @param [in] service_id Service to search.
        //! @return A safe pointer to the service context.
        //!
        ServiceContextPtr getService(uint16_t service_id);

    protected:

        // -------------------
        // Section description
        // -------------------

        //!
        //! This protected inner class contains the analysis context for one TID/TIDext into one PID.
        //!
        class TSDUCKDLL ETIDContext
        {
            TS_NOBUILD_NOCOPY(ETIDContext);
        public:
            // Public members - Synthetic data (do not modify outside ETIDContext methods)
            const ETID etid;                       //!< ETID value.
            uint64_t   table_count = 0;            //!< Number of occurences of this table (section# 0).
            uint64_t   section_count = 0;          //!< Number of occurences of sections in this table.
            uint64_t   repetition_ts = 0;          //!< Average number of TS packets between occurences of this table (section# 0).
            uint64_t   min_repetition_ts = 0;      //!< Minimum number of TS packets between occurences of this table (section# 0).
            uint64_t   max_repetition_ts = 0;      //!< Maximum number of TS packets between occurences of this table (section# 0).
            uint8_t    first_version = 0;          //!< First version encountered.
            uint8_t    last_version = 0;           //!< First version encountered.
            std::bitset<SVERSION_MAX> versions {}; //!< Set of versions.

            // Public members - Analysis data: Repetition interval evaluation:
            uint64_t   first_pkt = 0;              //!< Last packet index of first section# 0.
            uint64_t   last_pkt = 0;               //!< Last packet index of last section# 0.

            //!
            //! Constructor.
            //! @param [in] ext Extended table id.
            //!
            ETIDContext(const ETID& ext) : etid(ext) {}
        };

        //!
        //! Safe pointer to an ETIDContext (not thread-safe).
        //!
        typedef SafePtr<ETIDContext, ts::null_mutex> ETIDContextPtr;

        //!
        //! Map of ETIDContext, indexed by ETID.
        //!
        typedef std::map<ETID, ETIDContextPtr> ETIDContextMap;

        //!
        //! Get an ETID context.
        //! Allocate a new entry if the ETID is not found.
        //! @param [in] section A section containing the ETID to search.
        //! @return A safe pointer to the ETID context.
        //!
        ETIDContextPtr getETID(const Section& section);

    protected:

        // -------------------
        // PID description
        // -------------------

        //!
        //! This protected inner class contains the analysis context for one PID.
        //!
        class TSDUCKDLL PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            // Public members - Synthetic data (do not modify outside PIDContext methods)
            const PID     pid;                     //!< PID value.
            UString       description {};          //!< Readable description string (ie "MPEG-2 Audio").
            UString       comment {};              //!< Additional description (ie "MPE", "HbbTV").
            UStringVector languages {};            //!< For audio or subtitles (3 chars per language).
            UStringVector attributes {};           //!< Audio or video attributes (several lines if attributes changed).
            ServiceIdSet  services {};             //!< List of service ids the PID belongs to.
            bool          is_pmt_pid = false;      //!< Is the PMT PID for this service.
            bool          is_pcr_pid = false;      //!< Is the PCR PID for this service.
            bool          referenced = false;      //!< Is referenced (by service or global).
            bool          optional = false;        //!< Optional PID, don't display report if no packet.
            bool          carry_pes = false;       //!< This PID carries PES packets.
            bool          carry_section = false;   //!< This PID carries sections.
            bool          carry_ecm = false;       //!< This PID carries ECM's.
            bool          carry_emm = false;       //!< This PID carries EMM's.
            bool          carry_audio = false;     //!< This PID carries audio data.
            bool          carry_video = false;     //!< This PID carries video data.
            bool          carry_t2mi = false;      //!< Carry T2-MI encasulated data.
            bool          scrambled = false;       //!< Contains some scrambled packets.
            bool          same_stream_id = false;  //!< All PES packets have same stream_id.
            uint8_t       pes_stream_id = 0;       //!< Stream_id in PES packets on this PID.
            uint8_t       stream_type = 0;         //!< Stream type in PMT.
            uint64_t      ts_pkt_cnt = 0;          //!< Number of TS packets.
            uint64_t      ts_af_cnt = 0;           //!< Number of TS packets with adaptation field.
            uint64_t      unit_start_cnt = 0;      //!< Number of unit_start in packets.
            uint64_t      pl_start_cnt = 0;        //!< Number of unit_start & has_payload in packets.
            uint64_t      pmt_cnt = 0;             //!< Number of PMT (for PMT PID's).
            uint64_t      crypto_period = 0;       //!< Average number of TS packets per crypto-period.
            uint64_t      unexp_discont = 0;       //!< Number of unexpected discontinuities.
            uint64_t      exp_discont = 0;         //!< Number of expected discontinuities.
            uint64_t      duplicated = 0;          //!< Number of duplicated packets.
            uint64_t      ts_sc_cnt = 0;           //!< Number of scrambled packets.
            uint64_t      inv_ts_sc_cnt = 0;       //!< Number of invalid scrambling control in TS headers.
            uint64_t      inv_sections = 0;        //!< Number of invalid sections.
            uint64_t      inv_pes = 0;             //!< Number of invalid PES packets.
            uint64_t      inv_pes_start = 0;       //!< Number of invalid PES start code.
            uint64_t      t2mi_cnt = 0;            //!< Number of T2-MI packets.
            uint64_t      first_pcr = INVALID_PCR; //!< First PCR value in the PID, if any.
            uint64_t      last_pcr = INVALID_PCR;  //!< Last PCR value in the PID, if any.
            uint64_t      first_pts = INVALID_PTS; //!< First PTS value in the PID, if any.
            uint64_t      last_pts = INVALID_PTS;  //!< Last PTS value in the PID, if any.
            uint64_t      first_dts = INVALID_DTS; //!< First DTS value in the PID, if any.
            uint64_t      last_dts = INVALID_DTS;  //!< Last DTS value in the PID, if any.
            uint64_t      pcr_cnt = 0;             //!< Number of PCR's.
            uint64_t      pts_cnt = 0;             //!< Number of PTS's.
            uint64_t      dts_cnt = 0;             //!< Number of DTS's.
            uint64_t      pcr_leap_cnt = 0;        //!< Number of leaps in PCR's (potential time discontinuities).
            uint64_t      pts_leap_cnt = 0;        //!< Number of leaps in PTS's (potential time discontinuities).
            uint64_t      dts_leap_cnt = 0;        //!< Number of leaps in DTS's (potential time discontinuities).
            BitRate       ts_pcr_bitrate = 0;      //!< Average TS bitrate in b/s (eval from PCR).
            BitRate       bitrate = 0;             //!< Average PID bitrate in b/s.
            uint16_t      cas_id = 0;              //!< For EMM and ECM streams.
            std::set<uint32_t>         cas_operators {}; //!< Operators for EMM and ECM streams, when applicable.
            ETIDContextMap             sections {};      //!< List of sections in this PID.
            std::set<uint32_t>         ssu_oui {};       //!< Set of applicable OUI's for SSU.
            std::map<uint8_t,uint64_t> t2mi_plp_ts {};   //!< For T2-MI streams, map key = PLP (Physical Layer Pipe) to value = number of embedded TS packets.

            // Public members - Analysis data:
            uint8_t       cur_continuity = 0;   //!< Current continuity count.
            MPEG2AudioAttributes audio2 {};     //!< Last MPEG-2 audio attributes.

            // Public members - Analysis data: Crypto-period evaluation:
            uint8_t       cur_ts_sc = 0;        //!< Current scrambling control in TS header.
            uint64_t      cur_ts_sc_pkt = 0;    //!< First packet index of current crypto-period.
            uint64_t      cryptop_cnt = 0;      //!< Number of crypto-periods.
            uint64_t      cryptop_ts_cnt = 0;   //!< Number of TS packets in all crypto-periods.

            // Public members - Analysis data: Bitrate evaluation
            uint64_t      br_last_pcr = INVALID_PCR; //!< Last PCR value in the PID, for bitrate computation.
            uint64_t      br_last_pcr_pkt = 0;       //!< Index of packet with last PCR.
            BitRate       ts_bitrate_sum = 0;        //!< Sum of all computed TS bitrates.
            uint64_t      ts_bitrate_cnt = 0;        //!< Number of computed TS bitrates.

            //!
            //! Default constructor.
            //! @param [in] pid PID value.
            //! @param [in] description PID description.
            //!
            PIDContext(PID pid, const UString& description = UNREFERENCED);

            //!
            //! Register a service id for the PID.
            //! @param [in] service_id A service id which references the PID.
            //!
            void addService(uint16_t service_id);

            //!
            //! Return a full description, with comment and optionally attributes.
            //! @param [in] include_attributes Include the PID attributes in the description.
            //! @return The PID description.
            //!
            UString fullDescription(bool include_attributes) const;

        private:
            // Description of a few known PID's
            class KnownPID
            {
            public:
                const UChar* name;
                bool optional;
                bool sections;
            };
            typedef std::map<PID, KnownPID> KnownPIDMap;
            static KnownPIDMap::value_type KPID(PID, const UChar* name, bool optional = true, bool sections = true);
            static const KnownPIDMap KNOWN_PIDS;
        };

        //!
        //! Safe pointer to a PIDContext (not thread-safe).
        //!
        typedef SafePtr<PIDContext, ts::null_mutex> PIDContextPtr;

        //!
        //! Map of PIDContext, indexed by PID.
        //!
        typedef std::map<PID, PIDContextPtr> PIDContextMap;

        //!
        //! Check if a PID context exists.
        //! @param [in] pid PID to search.
        //! @return True if the PID exists, false otherwise.
        //!
        bool pidExists(PID pid) const;

        //!
        //! Get a PID context.
        //! Allocate a new entry if PID not found.
        //! @param [in] pid PID to search.
        //! @param [in] description Initial description of the PID if the context is created.
        //! @return A safe pointer to the PID context.
        //!
        PIDContextPtr getPID(PID pid, const UString& description = UNREFERENCED);

    protected:

        // ----------------------------
        // Transport stream description
        // ----------------------------

        //!
        //! Update the global statistics value if internal data were modified.
        //!
        void recomputeStatistics();

        // TSAnalyzer protected members.
        // Accessible to subclasses, valid after calling recomputeStatistics().
        // Important: subclasses shall not modify these fields, just read them.
        DuckContext&         _duck;                   //!< TSDuck execution context
        std::optional<uint16_t> _ts_id {};            //!< Transport stream id.
        uint64_t             _ts_pkt_cnt = 0;         //!< Number of TS packets.
        uint64_t             _invalid_sync = 0;       //!< Number of packets with invalid sync byte (not 0x47).
        uint64_t             _transport_errors = 0;   //!< Number of packets with transport error.
        uint64_t             _suspect_ignored = 0;    //!< Number of suspect packets, ignored.
        size_t               _pid_cnt = 0;            //!< Number of PID's (with actual packets).
        size_t               _scrambled_pid_cnt = 0;  //!< Number of scrambled PID's.
        size_t               _pcr_pid_cnt = 0;        //!< Number of PID's with PCR's.
        size_t               _global_pid_cnt = 0;     //!< Number of global PID's (ref but no service).
        size_t               _global_scr_pids = 0;    //!< Number of scrambled global PID's.
        uint64_t             _global_pkt_cnt = 0;     //!< Number of packets in global PID's.
        BitRate              _global_bitrate = 0;     //!< Bitrate for global PID's.
        size_t               _psisi_pid_cnt = 0;      //!< Number of global PSI/SI PID's (0x00 to 0x1F).
        size_t               _psisi_scr_pids = 0;     //!< Number of scrambled global PSI/SI PID's (normally zero).
        uint64_t             _psisi_pkt_cnt = 0;      //!< Number of packets in global PSI/SI PID's.
        BitRate              _psisi_bitrate = 0;      //!< Bitrate for global PSI/SI PID's.
        size_t               _unref_pid_cnt = 0;      //!< Number of unreferenced PID's.
        size_t               _unref_scr_pids = 0;     //!< Number of scrambled unreferenced PID's.
        uint64_t             _unref_pkt_cnt = 0;      //!< Number of packets in unreferenced PID's.
        BitRate              _unref_bitrate = 0;      //!< Bitrate for unreferenced PID's.
        BitRate              _ts_pcr_bitrate_188 = 0; //!< Average TS bitrate in b/s (eval from PCR).
        BitRate              _ts_pcr_bitrate_204 = 0; //!< Average TS bitrate in b/s (eval from PCR).
        BitRate              _ts_user_bitrate = 0;    //!< User-specified TS bitrate (if any).
        BitRateConfidence    _ts_user_br_confidence = BitRateConfidence::LOW;  //!< Confidence in user-specified TS bitrate.
        BitRate              _ts_bitrate = 0;         //!< TS bitrate (either from PCR or options).
        MilliSecond          _duration = 0;           //!< Total broadcast duration.
        Time                 _first_utc {};           //!< First system UTC time (first packet).
        Time                 _last_utc {};            //!< Last system UTC time (recomputeStatistics).
        Time                 _first_local {};         //!< First system local time (first packet).
        Time                 _last_local {};          //!< Last system local time (recomputeStatistics).
        Time                 _first_tdt {};           //!< First TDT UTC time stamp.
        Time                 _last_tdt {};            //!< Last TDT UTC time stamp.
        Time                 _first_tot {};           //!< First TOT local time stamp.
        Time                 _last_tot {};            //!< Last TOT local time stamp.
        Time                 _first_stt {};           //!< First STT (ATCS) UTC time stamp.
        Time                 _last_stt {};            //!< Last STT (ATCS) time stamp.
        UString              _country_code {};        //!< TOT country code.
        uint16_t             _scrambled_services_cnt = 0; //!< Number of scrambled services.
        std::bitset<TID_MAX> _tid_present {};         //!< Array of detected tables.
        PIDContextMap        _pids {};                //!< Description of PIDs.
        ServiceContextMap    _services {};            //!< Description of services, map key: service id.

    private:
        // Constant string "Unreferenced"
        static const UString UNREFERENCED;

        // Reset the section demux.
        void resetSectionDemux();

        // Analyze the various PSI tables
        void analyzePAT(const PAT&);
        void analyzeCAT(const CAT&);
        void analyzePMT(PID pid, const PMT&);
        void analyzeNIT(PID pid, const NIT&);
        void analyzeSDT(const SDT&);
        void analyzeTDT(const TDT&);
        void analyzeTOT(const TOT&);
        void analyzeMGT(const MGT&);
        void analyzeVCT(const VCT&);
        void analyzeSTT(const STT&);

        // Analyse a list of descriptors.
        // If svp is not 0, we are in the PMT of the specified service.
        // If ps is not 0, we are in the description of this PID in a PMT.
        void analyzeDescriptors(const DescriptorList& descs, ServiceContext* svp = nullptr, PIDContext* ps = nullptr);

        // Analyse one CA descriptor, either from the CAT or a PMT.
        // If svp is not 0, we are in the PMT of the specified service.
        // If ps is not 0, we are in the description of this PID in a PMT.
        // If svp is 0, we are in the CAT.
        void analyzeCADescriptor(const Descriptor& desc, ServiceContext* svp = nullptr, PIDContext* ps = nullptr, const UString& suffix = UString());

        // Implementation of TableHandlerInterface
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Implementation of SectionHandlerInterface
        virtual void handleSection(SectionDemux&, const Section&) override;

        // Implementation of InvalidSectionHandlerInterface
        virtual void handleInvalidSection(SectionDemux&, const DemuxedData&) override;

        // Implementation of PESHandlerInterface
        virtual void handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket&, const MPEG2AudioAttributes&) override;
        virtual void handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket&, const MPEG2VideoAttributes&) override;
        virtual void handleNewAVCAttributes(PESDemux&, const PESPacket&, const AVCAttributes&) override;
        virtual void handleNewHEVCAttributes(PESDemux&, const PESPacket&, const HEVCAttributes&) override;
        virtual void handleNewAC3Attributes(PESDemux&, const PESPacket&, const AC3Attributes&) override;
        virtual void handleInvalidPESPacket(PESDemux&, const DemuxedData&) override;

        // Implementation of T2MIHandlerInterface
        virtual void handleT2MINewPID(T2MIDemux& demux, const PMT& pmt, PID pid, const T2MIDescriptor& desc) override;
        virtual void handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt) override;
        virtual void handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts) override;

        // TSAnalyzer private members (state data, used during analysis):
        bool         _modified = false;              // Internal data modified, need recomputeStatistics
        BitRate      _ts_bitrate_sum = 0;            // Sum of all computed TS bitrates
        uint64_t     _ts_bitrate_cnt = 0;            // Number of computed TS bitrates
        uint64_t     _preceding_errors = 0;          // Number of contiguous invalid packets before current packet
        uint64_t     _preceding_suspects = 0;        // Number of contiguous suspects packets before current packet
        uint64_t     _min_error_before_suspect = 1;  // Required number of invalid packets before starting suspect
        uint64_t     _max_consecutive_suspects = 1;  // Max number of consecutive suspect packets before clearing suspect
        SectionDemux _demux {_duck, this, this};     // PSI tables analysis
        PESDemux     _pes_demux {_duck, this};       // Audio/video analysis
        T2MIDemux    _t2mi_demux {_duck, this};      // T2-MI analysis
        LogicalChannelNumbers _lcn {_duck};          // Accumulate LCN and visible flags
    };
}
