//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to analyze PES packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsFileNameGenerator.h"
#include "tsPESDemux.h"
#include "tsStdio.h"

namespace ts {
    //!
    //! Plugin to analyze PES packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PESPlugin: public ProcessorPlugin, private PESHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(PESPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Commmand line options.
        bool      _trace_packets = false;
        bool      _trace_packet_index = false;
        bool      _dump_pes_header = false;
        bool      _dump_pes_payload = false;
        bool      _dump_start_code = false;
        bool      _dump_nal_units = false;
        bool      _dump_avc_sei = false;
        bool      _video_attributes = false;
        bool      _audio_attributes = false;
        bool      _intra_images = false;
        bool      _negate_nal_unit_filter = false;
        bool      _multiple_files = false;
        bool      _flush_last = false;
        uint32_t  _hexa_flags = 0;
        size_t    _hexa_bpl = 0;
        size_t    _max_dump_size = 0;
        size_t    _max_dump_count = 0;
        int       _min_payload = 0;    // Minimum payload size (<0: no filter)
        int       _max_payload = 0;    // Maximum payload size (<0: no filter)
        fs::path  _out_filename {};
        fs::path  _pes_filename {};
        fs::path  _es_filename {};
        PIDSet    _pids {};
        CodecType _default_h26x = CodecType::UNDEFINED;
        std::set<uint8_t>    _nal_unit_filter {};
        std::set<uint32_t>   _sei_type_filter {};
        std::list<ByteBlock> _sei_uuid_filter {};

        // Working data.
        bool              _abort = false;
        std::ofstream     _out_file {};
        std::ostream*     _out = nullptr;
        std::ofstream     _pes_file {};
        std::ostream*     _pes_stream = nullptr;
        std::ofstream     _es_file {};
        std::ostream*     _es_stream = nullptr;
        Stdio::BinaryMode _out_mode {this, Stdio::STDOUT};
        PESDemux          _demux;
        FileNameGenerator _pes_name_gen {};
        FileNameGenerator _es_name_gen {};

        // Open output file.
        bool openOutput(const fs::path&, std::ofstream*, std::ostream**, bool binary);

        // A string containing the PID and optional TS packet indexes.
        UString prefix(const DemuxedData&) const;

        // Do we need to display this acces unit type?
        bool useAccesUnitType(uint8_t) const;

        // Process dump count. Return true when terminated. Also process error on output.
        bool lastDump(std::ostream&);

        // Save one file using --multiple-file. Set _abort on error.
        void saveOnePES(FileNameGenerator& namegen, const uint8_t* data, size_t size);

        // Implementation of PESHandlerInterface.
        virtual void handlePESPacket(PESDemux&, const PESPacket&) override;
        virtual void handleInvalidPESPacket(PESDemux&, const DemuxedData&) override;
        virtual void handleIntraImage(PESDemux&, const PESPacket&, size_t) override;
        virtual void handleVideoStartCode(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket&, const MPEG2VideoAttributes&) override;
        virtual void handleAccessUnit(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleSEI(PESDemux& demux, const PESPacket& packet, uint32_t sei_type, size_t offset, size_t size) override;
        virtual void handleNewAVCAttributes(PESDemux&, const PESPacket&, const AVCAttributes&) override;
        virtual void handleNewHEVCAttributes(PESDemux&, const PESPacket&, const HEVCAttributes&) override;
        virtual void handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket&, const MPEG2AudioAttributes&) override;
        virtual void handleNewAC3Attributes(PESDemux&, const PESPacket&, const AC3Attributes&) override;
    };
}
