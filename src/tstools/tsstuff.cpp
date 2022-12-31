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
//  Add stuffing to a TS file to reach a target bitrate.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSFileInputBuffered.h"
#include "tsVariable.h"
TS_MAIN(MainCode);


//-----------------------------------------------------------------------------
//  Command line options
//-----------------------------------------------------------------------------

static const size_t   MIN_TS_BUFFER_SIZE     = 1024;             // 1 kB
static const size_t   DEFAULT_TS_BUFFER_SIZE = 4 * 1024 * 1024;  // 4 MB
static const size_t   MAX_TS_BUFFER_SIZE     = 16 * 1024 * 1024; // 16 MB
static const uint64_t DEFAULT_MIN_INTERVAL   = 100;              // milliseconds

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::BitRate target_bitrate;
        ts::PID     reference_pid;
        size_t      buffer_size;
        uint64_t    leading_packets;
        uint64_t    trailing_packets;
        uint64_t    final_inter_packet;
        uint64_t    initial_inter_packet;
        uint64_t    min_interval_ms;
        bool        dts_based;
        bool        dyn_final_inter_packet;
        bool        dyn_initial_inter_packet;
        ts::UString input_file;
        ts::UString output_file;
        ts::TSPacketFormat input_format;
        ts::TSPacketFormat output_format;
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Add stuffing to a transport stream to reach a target bitrate", u"[options] [input-file]"),
    target_bitrate(0),
    reference_pid(ts::PID_NULL),
    buffer_size(0),
    leading_packets(0),
    trailing_packets(0),
    final_inter_packet(0),
    initial_inter_packet(0),
    min_interval_ms(0),
    dts_based(false),
    dyn_final_inter_packet(false),
    dyn_initial_inter_packet(false),
    input_file(),
    output_file(),
    input_format(ts::TSPacketFormat::AUTODETECT),
    output_format(ts::TSPacketFormat::TS)
{
    option(u"", 0, FILENAME, 0, 1);
    help(u"",
         u"The input file is a TS file, typically with variable bitrate content. "
         u"By default, the standard input is used.");

    option<ts::BitRate>(u"bitrate", 'b', 1, 1, 1);
    help(u"bitrate",
         u"Target constant bitrate of the output file. "
         u"This is mandatory parameter, there is no default.");

    option(u"buffer-size", 0, INTEGER, 0, 1, MIN_TS_BUFFER_SIZE, MAX_TS_BUFFER_SIZE);
    help(u"buffer-size",
         u"Input buffer size, in bytes. Must be large enough to always contain two "
         u"time stamps in the reference PID. Default: " + ts::UString::Decimal(DEFAULT_TS_BUFFER_SIZE) + u" bytes.");

    option(u"dts-based", 'd');
    help(u"dts-based",
         u"Use Decoding Time Stamps (DTS) in the reference PID to evaluate the "
         u"amount of stuffing to insert. The default is to use Program Clock "
         u"References (PCR) instead of DTS.");

    option(u"final-inter-packet", 'f', UNSIGNED);
    help(u"final-inter-packet",
         u"Number of stuffing packets to add between input packets after the last "
         u"time stamp (PCR or DTS). By default, use the same number as in the "
         u"previous segment, between the last two time stamps.");

    option(u"initial-inter-packet", 'i', UNSIGNED);
    help(u"initial-inter-packet",
         u"Number of stuffing packets to add between input packets before the first "
         u"time stamp (PCR or DTS). By default, use the same number as in the "
         u"first segment, between the first two time stamps.");

    option(u"leading-packets", 'l', UNSIGNED);
    help(u"leading-packets",
         u"Number of consecutive stuffing packets to add at the beginning of the "
         u"output file, before the first input packet. The default is zero.");

    option(u"min-interval", 'm', POSITIVE);
    help(u"min-interval",
         u"Minimum interval, in milli-seconds, between two recomputations of the "
         u"amount of stuffing to insert. This duration is based on time-stamps, "
         u"not real time. The default is " + ts::UString::Decimal(DEFAULT_MIN_INTERVAL) + u" ms.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Output file name (standard output by default). The output file is a TS "
         u"file with the same packets as the input file with interspersed stuffing "
         u"packets and a constant bitrate.");

    option(u"reference-pid", 'r', PIDVAL);
    help(u"reference-pid",
         u"PID in which to collect time stamps (PCR or DTS) to use as reference "
         u"for the insertion of stuffing packets. By default, use the first PID "
         u"containing the specified type of time stamps (PCR or DTS).");

    option(u"trailing-packets", 't', UNSIGNED);
    help(u"trailing-packets",
         u"Number of consecutive stuffing packets to add at the end of the "
         u"output file, after the last input packet. The default is zero.");

    ts::DefineTSPacketFormatInputOption(*this, 0, u"input-format");
    ts::DefineTSPacketFormatOutputOption(*this, 0, u"output-format");

    analyze(argc, argv);

    getValue(input_file, u"");
    getValue(output_file, u"output-file");

    input_format = ts::LoadTSPacketFormatInputOption(*this, u"input-format");
    output_format = ts::LoadTSPacketFormatOutputOption(*this, u"output-format");

    getValue(target_bitrate, u"bitrate", 0);
    assert(target_bitrate != 0);

    getIntValue(buffer_size, u"buffer-size", DEFAULT_TS_BUFFER_SIZE);
    dts_based = present(u"dts-based");
    getIntValue(reference_pid, u"reference-pid", ts::PID_NULL);
    getIntValue(final_inter_packet, u"final-inter-packet", 0);
    getIntValue(initial_inter_packet, u"initial-inter-packet", 0);
    getIntValue(leading_packets, u"leading-packets", 0);
    getIntValue(trailing_packets, u"trailing-packets", 0);
    getIntValue(min_interval_ms, u"min-interval", DEFAULT_MIN_INTERVAL);
    dyn_final_inter_packet = !present(u"final-inter-packet");
    dyn_initial_inter_packet = !present(u"initial-inter-packet");

    exitOnError();
}


//-----------------------------------------------------------------------------
// Define a time stamp in the input TS
//-----------------------------------------------------------------------------

struct TimeStamp
{
    uint64_t          tstamp;  // Time stamp in PCR units
    uint64_t packet;  // Packet index in input file

    // Constructor
    TimeStamp(uint64_t t = 0, uint64_t p = 0) : tstamp(t), packet(p) {}
};


//-----------------------------------------------------------------------------
// This class processes the input file
//-----------------------------------------------------------------------------

class Stuffer
{
    TS_NOBUILD_NOCOPY(Stuffer);
public:
    // Constructor
    Stuffer(Options&);
    virtual ~Stuffer();

    // Process the content
    void stuff();

private:
    // Private members
    Options&                _opt;                     // Command-line options.
    ts::TSFileInputBuffered _input;                   // Input file, including seek buffer for at least one segment.
    ts::TSFile              _output;                  // Output file.
    ts::Variable<TimeStamp> _tstamp1;                 // First time-stamp in current segment.
    ts::Variable<TimeStamp> _tstamp2;                 // Second time-stamp in current segment.
    uint64_t                _current_inter_packet;    // Number of null packets to add between all input packets in segment.
    uint64_t                _current_residue_packets; // Packets to add to inter_packets.
    uint64_t                _remaining_stuff_count;   // Remaining number of stuffing packets to add before end of segment.
    uint64_t                _additional_bits;         // Additional bits (less than one packet) to add in next segment.

    // Abort processing (invoked on fatal error, when message already reported)
    [[noreturn]] void fatalError();

    // Get name of time stamps
    ts::UString getTimeStampType() const {return _opt.dts_based ? u"DTS" : u"PCR";}

    // Check if a packet contains a time stamp.
    bool getTimeStamp(const ts::TSPacket& pkt, uint64_t& tstamp) const;

    // Evaluate stuffing need in next segment, between two time stamps.
    // Compute _current_inter_packet, _remaining_stuff_count, _additional_bits.
    void evaluateNextStuffing();

    // Write the specified number of stuffing packets
    void writeStuffing(uint64_t stuffing_packet_count);

    // Read input up to end_packet and perform simple inter-packet stuffing.
    void simpleInterPacketStuffing(uint64_t inter_packet, uint64_t end_packet);
};


//-----------------------------------------------------------------------------
// Stuffer constructors and destructors.
//-----------------------------------------------------------------------------

Stuffer::Stuffer(Options& opt) :
    _opt(opt),
    _input(opt.buffer_size / ts::PKT_SIZE),
    _output(),
    _tstamp1(),
    _tstamp2(),
    _current_inter_packet(0),
    _current_residue_packets(0),
    _remaining_stuff_count(0),
    _additional_bits(0)
{
}

Stuffer::~Stuffer()
{
}


//-----------------------------------------------------------------------------
// Abort processing
//-----------------------------------------------------------------------------

void Stuffer::fatalError()
{
    // Maybe something more clever some day...
    ::exit(EXIT_FAILURE);
}


//-----------------------------------------------------------------------------
// Check if a packet contains a time stamp.
//-----------------------------------------------------------------------------

bool Stuffer::getTimeStamp(const ts::TSPacket& pkt, uint64_t& tstamp) const
{
    if (_opt.dts_based) {
        if (pkt.hasDTS()) {
            tstamp = pkt.getDTS() * ts::SYSTEM_CLOCK_SUBFACTOR;
            return true;
        }
    }
    else {
        if (pkt.hasPCR()) {
            tstamp = pkt.getPCR();
            return true;
        }
    }
    return false;
}


//-----------------------------------------------------------------------------
// Write the specified number of stuffing packets
//-----------------------------------------------------------------------------

void Stuffer::writeStuffing(uint64_t count)
{
    while (count > 0) {
        if (!_output.writePackets(&ts::NullPacket, nullptr, 1, _opt)) {
            fatalError();
        }
        count--;
    }
}


//-----------------------------------------------------------------------------
// Read input up to end_packet and perform simple inter-packet stuffing.
//-----------------------------------------------------------------------------

void Stuffer::simpleInterPacketStuffing(uint64_t inter_packet, uint64_t end_packet)
{
    assert(_input.readPacketsCount() < end_packet);

    ts::TSPacket pkt;
    while (_input.readPacketsCount() < end_packet && _input.read(&pkt, 1, _opt) == 1) {
        if (!_output.writePackets(&pkt, nullptr, 1, _opt)) {
            fatalError();
        }
        writeStuffing(inter_packet);
    }
}


//-----------------------------------------------------------------------------
// Evaluate stuffing need in next segment, between two time stamps.
//-----------------------------------------------------------------------------

void Stuffer::evaluateNextStuffing()
{
    // Save initial position in the file
    const ts::PacketCounter initial_position = _input.readPacketsCount();
    _opt.debug(u"evaluateNextStuffing: initial_position = %'d", {initial_position});

    // Initialize new search. Note that _tstamp1 and _tstamp2 may be unset.
    _tstamp1 = _tstamp2;
    _tstamp2.clear();

    // Read packets until both _tstamp1 and _tstamp2 are set (or end of file)
    ts::TSPacket pkt;
    while (!_tstamp2.set() && _input.canSeek(initial_position) && _input.read(&pkt, 1, _opt) == 1) {
        uint64_t tstamp = 0;
        if (getTimeStamp(pkt, tstamp)) {
            if (_opt.reference_pid == ts::PID_NULL) {
                // Found the first time stamp, use this PID as reference
                _opt.reference_pid = pkt.getPID();
                _opt.verbose(u"using PID %d (0x%X) as reference", {_opt.reference_pid, _opt.reference_pid});
            }
            else if (_opt.reference_pid != pkt.getPID()) {
                // Not the reference PID, skip;
                continue;
            }
            const TimeStamp time_stamp(tstamp, _input.readPacketsCount());
            if (!_tstamp1.set() || tstamp < _tstamp1.value().tstamp) {
                // 1) Found the first time stamp in the file.
                // 2) Or found a time stamp lower than tstamp1, may be because of a
                //    file rewind or wrapping at 2**42. Use new time stamp as first.
                _tstamp1 = time_stamp;
                _opt.debug(u"evaluateNextStuffing: tstamp1 = %'d at %'d", {time_stamp.tstamp, time_stamp.packet});
            }
            else if (((tstamp - _tstamp1.value().tstamp) * 1000) / ts::SYSTEM_CLOCK_FREQ >= _opt.min_interval_ms) {
                // Found second time stamp (with a sufficiently large interval from first time stamp).
                _tstamp2 = time_stamp;
                _opt.debug(u"evaluateNextStuffing: tstamp2 = %'d at %'d", {time_stamp.tstamp, time_stamp.packet});
            }
        }
    }

    // If _tstamp2 not set in first segment or after buffer full, we cannot perform bitrate evaluation
    if (!_tstamp2.set() && (initial_position == 0 || !_input.canSeek(initial_position))) {
        ts::UString msg(u"no " + getTimeStampType() + u" found");
        if (initial_position > 0) {
            msg += ts::UString::Format(u" after packet %'d", {initial_position});
        }
        if (_opt.reference_pid != ts::PID_NULL) {
            msg += ts::UString::Format(u" in PID %d (0x%X), try another PID or increasing --buffer-size", {_opt.reference_pid, _opt.reference_pid});
        }
        else {
            msg += u", try increasing --buffer-size";
        }
        _opt.fatal(msg);
    }

    // Restore initial position in the file
    if (!_input.seekBackward(size_t(_input.readPacketsCount() - initial_position), _opt)) {
        fatalError();
    }

    if (_tstamp2.set()) {
        // The segment is defined by the two time stamps. Compute new settings.
        assert(_tstamp1.set());
        assert(_tstamp1.value().tstamp < _tstamp2.value().tstamp);
        assert(_tstamp1.value().packet < _tstamp2.value().packet);

        // Segment duration in PCR unit:
        const uint64_t duration = _tstamp2.value().tstamp - _tstamp1.value().tstamp;

        // Actual number of input packets in the segment and corresponding bitrate.
        const uint64_t input_packets = _tstamp2.value().packet - _tstamp1.value().packet;
        const ts::BitRate input_bitrate = ts::BitRate(input_packets * ts::PKT_SIZE_BITS * ts::SYSTEM_CLOCK_FREQ) / duration;
        _opt.debug(u"segment: %'d ms, %'d packets, %'d b/s", {(duration * 1000) / ts::SYSTEM_CLOCK_FREQ, input_packets, input_bitrate});

        // Target output number of bits in segment, plus the previously unstuffed bits (less than one packet).
        const uint64_t target_bits = _additional_bits + ((_opt.target_bitrate * duration) / ts::SYSTEM_CLOCK_FREQ).toInt();

        // Target output number of packets in the segment.
        const uint64_t target_packets = target_bits / ts::PKT_SIZE_BITS;

        // Compute number of stuffing packets to add
        if (input_packets > target_packets) {
            // Input burst greater than target bitrate.
            _opt.warning(u"input bitrate locally higher (%'d b/s) than target bitrate, cannot stuff", {input_bitrate});
            _remaining_stuff_count = 0;
            _current_inter_packet = 0;
            _current_residue_packets = 0;
            _additional_bits = 0;
        }
        else {
            _remaining_stuff_count = target_packets - input_packets;
            _current_inter_packet = _remaining_stuff_count / input_packets;
            _current_residue_packets = _remaining_stuff_count % input_packets;
            _additional_bits = target_bits % ts::PKT_SIZE_BITS;
        }
    }
    else {
        // _tstamp2 is not set, we reached the end of file, keep previous settings.
        // Keep same inter-packet stuffing but erase fine tuning values.
        // This means that we do not do anything if the new bitrate is not at least
        // the double of the input one.
        _remaining_stuff_count = 0;
        _current_residue_packets = 0;
        _additional_bits = 0;
    }
}


//-----------------------------------------------------------------------------
// Process the content
//-----------------------------------------------------------------------------

void Stuffer::stuff()
{
    // Open input file
    if (!_input.openRead(_opt.input_file, 1, 0, _opt, _opt.input_format)) {
        fatalError();
    }

    _opt.debug(u"input file buffer size: %'d packets", {_input.getBufferSize()});

    // Remaining number of bits to stuff representing less than one packet
    _additional_bits = 0;

    // Locate first two time stamps,
    _tstamp1.clear();
    _tstamp2.clear();
    evaluateNextStuffing();
    assert(_tstamp1.set());
    assert(_tstamp2.set());

    // Create output file
    if (!_output.open(_opt.output_file, ts::TSFile::WRITE | ts::TSFile::SHARED, _opt, _opt.output_format)) {
        fatalError();
    }

    // Write leading stuffing packets
    writeStuffing(_opt.leading_packets);

    // Perform initial stuffing, up to the first time stamp
    simpleInterPacketStuffing(_opt.dyn_initial_inter_packet ? _current_inter_packet : _opt.initial_inter_packet, _tstamp1.value().packet);

    // Perform stuffing, segment after segment
    while (_tstamp2.set()) {
        assert(_input.readPacketsCount() < _tstamp2.value().packet);

        // Perform stuffing on current segment, loop on input packets, one by one.
        ts::TSPacket pkt;
        while (_input.readPacketsCount() < _tstamp2.value().packet && _input.read(&pkt, 1, _opt) == 1) {
            // Write the input packet.
            if (!_output.writePackets(&pkt, nullptr, 1, _opt)) {
                fatalError();
            }
            // Write stuffing packets after each input packet.
            const uint64_t residue = _current_residue_packets > 0 ? 1 : 0;
            const uint64_t count = std::min(_current_inter_packet + residue, _remaining_stuff_count);
            writeStuffing(count);
            _remaining_stuff_count -= count;
            _current_residue_packets -= residue;
        }

        // Optionally write a burst of null packets at the end of segment.
        writeStuffing(_remaining_stuff_count);
        _remaining_stuff_count = 0;

        // Evaluate stuffing need for next segment
        evaluateNextStuffing();
    }

    // Perform final stuffing, up to end of file
    simpleInterPacketStuffing(_opt.dyn_final_inter_packet ? _current_inter_packet : _opt.final_inter_packet, std::numeric_limits<uint64_t>::max());

    // Write trailing stuffing packets
    writeStuffing(_opt.trailing_packets);

    _opt.verbose(u"stuffing completed, read %'d packets, written %'d packets", {_input.readPacketsCount(), _output.writePacketsCount()});

    // Close files
    _output.close(_opt);
    _input.close(_opt);
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    Stuffer stuffer(opt);
    stuffer.stuff();
    return EXIT_SUCCESS;
}
