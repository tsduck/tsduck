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
//  Add stuffing to a TS file to reach a target bitrate.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsTSFileInputBuffered.h"
#include "tsTSFileOutput.h"
#include "tsVariable.h"
#include "tsDecimal.h"
#include "tsFormat.h"

using namespace ts;

namespace {

    //-------------------------------------------------------------------------
    //  Command line options
    //-------------------------------------------------------------------------

    static const size_t MIN_TS_BUFFER_SIZE     = 1024;             // 1 kB
    static const size_t DEFAULT_TS_BUFFER_SIZE = 4 * 1024 * 1024;  // 4 MB
    static const size_t MAX_TS_BUFFER_SIZE     = 16 * 1024 * 1024; // 16 MB

    struct Options: public Args
    {
        Options (int argc, char *argv[]);

        BitRate       target_bitrate;
        ts::PID     reference_pid;
        size_t        buffer_size;
        PacketCounter leading_packets;
        PacketCounter trailing_packets;
        PacketCounter final_inter_packet;
        PacketCounter initial_inter_packet;
        bool          dts_based;
        bool          dyn_final_inter_packet;
        bool          dyn_initial_inter_packet;
        std::string   input_file;
        std::string   output_file;
    };

    // Constructor
    Options::Options (int argc, char *argv[]) :
        Args ("Add stuffing to a TS file to reach a target bitrate.", "[options] [input-file]")
    {
        option ("",                      0,  Args::STRING, 0, 1);
        option ("bitrate",              'b', Args::POSITIVE, 1, 1);
        option ("buffer-size",           0,  Args::INTEGER, 0, 1, MIN_TS_BUFFER_SIZE, MAX_TS_BUFFER_SIZE);
        option ("debug",                 0,  Args::POSITIVE, 0, 1, 0, 0, true);
        option ("dts-based",            'd');
        option ("final-inter-packet",   'f', Args::UNSIGNED);
        option ("initial-inter-packet", 'i', Args::UNSIGNED);
        option ("leading-packets",      'l', Args::UNSIGNED);
        option ("output-file",          'o', Args::STRING);
        option ("reference-pid",        'r', Args::PIDVAL);
        option ("trailing-packets",     't', Args::UNSIGNED);
        option ("verbose",              'v');

        setHelp ("Input file:\n"
                 "  The input file is a TS file, typically with variable bitrate content.\n"
                 "  By default, the standard input is used.\n"
                 "\n"
                 "Options:\n"
                 "\n"
                 "  -b value\n"
                 "  --bitrate value\n"
                 "      Target constant bitrate of the output file. This is mandatory parameter,\n"
                 "      there is no default.\n"
                 "\n"
                 "  --buffer-size value\n"
                 "      Input buffer size, in bytes. Must be large enough to always contain two\n"
                 "      time stamps in the reference PID. Default: " + Decimal (DEFAULT_TS_BUFFER_SIZE) + " bytes.\n"
                 "\n"
                 "  -d\n"
                 "  --dts-based\n"
                 "      Use Decoding Time Stamps (DTS) in the reference PID to evaluate the\n"
                 "      amount of stuffing to insert. The default is to use Program Clock\n"
                 "      References (PCR) instead of DTS.\n"
                 "\n"
                 "  -f value\n"
                 "  --final-inter-packet value\n"
                 "      Number of stuffing packets to add between input packets after the last\n"
                 "      time stamp (PCR or DTS). By default, use the same number as in the\n"
                 "      previous segment, between the last two time stamps.\n"
                 "\n"
                 "  --help\n"
                 "      Display this help text.\n"
                 "\n"
                 "  -i value\n"
                 "  --initial-inter-packet value\n"
                 "      Number of stuffing packets to add between input packets before the first\n"
                 "      time stamp (PCR or DTS). By default, use the same number as in the\n"
                 "      first segment, between the first two time stamps.\n"
                 "\n"
                 "  -l value\n"
                 "  --leading-packets value\n"
                 "      Number of consecutive stuffing packets to add at the beginning of the\n"
                 "      output file, before the first input packet. The default is zero.\n"
                 "\n"
                 "  -o filename\n"
                 "  --output filename\n"
                 "      Output file name (standard output by default). The output file is a TS\n"
                 "      file with the same packets as the input file with interspersed stuffing\n"
                 "      packets and a constant bitrate.\n"
                 "\n"
                 "  -r value\n"
                 "  --reference-pid value\n"
                 "      PID in which to collect time stamps (PCR or DTS) to use as reference\n"
                 "      for the insertion of stuffing packets. By default, use the first PID\n"
                 "      containing the specified type of time stamps (PCR or DTS).\n"
                 "\n"
                 "  -t value\n"
                 "  --trailing-packets value\n"
                 "      Number of consecutive stuffing packets to add at the end of the\n"
                 "      output file, after the last input packet. The default is zero.\n"
                 "\n"
                 "  -v\n"
                 "  --verbose\n"
                 "      Produce verbose output.\n"
                 "\n"
                 "  --version\n"
                 "      Display the version number.\n");

        analyze (argc, argv);

        if (present ("debug")) {
            setDebugLevel (intValue ("debug", Severity::Debug));
        }
        else {
            setDebugLevel (present ("verbose") ? Severity::Verbose : Severity::Info);
        }

        getValue (input_file, "");
        getValue (output_file, "output-file");

        target_bitrate = intValue<BitRate> ("bitrate", 0);
        assert (target_bitrate != 0);

        buffer_size = intValue<size_t> ("buffer-size", DEFAULT_TS_BUFFER_SIZE);
        dts_based = present ("dts-based");
        reference_pid = intValue<ts::PID> ("reference-pid", PID_NULL);
        final_inter_packet = intValue<PacketCounter> ("final-inter-packet", 0);
        initial_inter_packet = intValue<PacketCounter> ("initial-inter-packet", 0);
        leading_packets = intValue<PacketCounter> ("leading-packets", 0);
        trailing_packets = intValue<PacketCounter> ("trailing-packets", 0);
        dyn_final_inter_packet = !present ("final-inter-packet");
        dyn_initial_inter_packet = !present ("initial-inter-packet");

        exitOnError ();
    }


    //-------------------------------------------------------------------------
    // Define a time stamp in the input TS
    //-------------------------------------------------------------------------

    struct TimeStamp
    {
        uint64_t        tstamp;  // Time stamp in PCR units
        PacketCounter packet;  // Packet index in input file

        // Constructor
        TimeStamp (uint64_t t = 0, PacketCounter p = 0) : tstamp (t), packet (p) {}
    };


    //-------------------------------------------------------------------------
    // This class processes the input file
    //-------------------------------------------------------------------------

    class Stuffer
    {
    public:
        // Constructor
        Stuffer (Options&);
        virtual ~Stuffer() {}

        // Process the content
        void stuff();

    private:
        // Private members
        Options&            _opt;
        TSFileInputBuffered _input;
        TSFileOutput        _output;
        PacketCounter       _current_inter_packet;
        PacketCounter       _remaining_stuff_count;
        PacketCounter       _additional_bits;
        Variable<TimeStamp> _tstamp1;
        Variable<TimeStamp> _tstamp2;

        // Abort processing (invoked on fatal error, when message already reported)
        void fatalError();

        // Get name of time stamps
        std::string getTimeStampType() const {return _opt.dts_based ? "DTS" : "PCR";}

        // Check if a packet contains a time stamp.
        bool getTimeStamp (const TSPacket& pkt, uint64_t& tstamp) const;

        // Evaluate stuffing need in next segment, between two time stamps.
        void evaluateNextStuffing();

        // Write the specified number of stuffing packets
        void writeStuffing (PacketCounter stuffing_packet_count);

        // Read input up to end_packet and perform simple inter-packet stuffing.
        void simpleInterPacketStuffing (PacketCounter inter_packet, PacketCounter end_packet);
    };


    //-------------------------------------------------------------------------
    // Stuffer constructor
    //-------------------------------------------------------------------------

    Stuffer::Stuffer (Options& opt) :
        _opt (opt),
        _input (opt.buffer_size / PKT_SIZE)
    {
    }


    //-------------------------------------------------------------------------
    // Abort processing
    //-------------------------------------------------------------------------

    void Stuffer::fatalError()
    {
        // Maybe something more clever some day...
        ::exit (EXIT_FAILURE);
    }


    //-------------------------------------------------------------------------
    // Check if a packet contains a time stamp.
    //-------------------------------------------------------------------------

    bool Stuffer::getTimeStamp (const TSPacket& pkt, uint64_t& tstamp) const
    {
        if (_opt.dts_based) {
            if (pkt.hasDTS()) {
                tstamp = pkt.getDTS() * SYSTEM_CLOCK_SUBFACTOR;
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


    //-------------------------------------------------------------------------
    // Write the specified number of stuffing packets
    //-------------------------------------------------------------------------

    void Stuffer::writeStuffing (PacketCounter count)
    {
        while (count > 0) {
            if (!_output.write (&NullPacket, 1, _opt)) {
                fatalError();
            }
            count--;
        }
    }


    //-------------------------------------------------------------------------
    // Read input up to end_packet and perform simple inter-packet stuffing.
    //-------------------------------------------------------------------------

    void Stuffer::simpleInterPacketStuffing (PacketCounter inter_packet, PacketCounter end_packet)
    {
        assert (_input.getPacketCount() < end_packet);

        TSPacket pkt;
        while (_input.getPacketCount() < end_packet && _input.read (&pkt, 1, _opt) == 1) {
            if (!_output.write (&pkt, 1, _opt)) {
                fatalError();
            }
            writeStuffing (inter_packet);
        }
    }


    //-------------------------------------------------------------------------
    // Evaluate stuffing need in next segment, between two time stamps.
    //-------------------------------------------------------------------------

    void Stuffer::evaluateNextStuffing()
    {
        // Save initial position in the file
        const PacketCounter initial_position = _input.getPacketCount();
        _opt.debug ("evaluateNextStuffing: initial_position = " + Decimal (initial_position));

        // Initialize new search. Note that _tstamp1 and _tstamp2 may be unset.
        _tstamp1 = _tstamp2;
        _tstamp2.reset();

        // Read packets until both _tstamp1 and _tstamp2 are set (or end of file)
        uint64_t tstamp;
        TSPacket pkt;
        while (!_tstamp2.set() && _input.canSeek (initial_position) && _input.read (&pkt, 1, _opt) == 1) {
            if (getTimeStamp (pkt, tstamp)) {
                if (_opt.reference_pid == PID_NULL) {
                    // Found the first time stamp, use this PID as reference
                    _opt.reference_pid = pkt.getPID();
                    _opt.verbose ("using PID %d (0x%04X) as reference", int (_opt.reference_pid), int (_opt.reference_pid));
                }
                else if (_opt.reference_pid != pkt.getPID()) {
                    // Not the reference PID, skip;
                    continue;
                }
                const TimeStamp time_stamp (tstamp, _input.getPacketCount());
                if (!_tstamp1.set() || tstamp <= _tstamp1.value().tstamp) {
                    // 1) Found the first time stamp in the file.
                    // 2) Or found a time stamp lower than tstamp1, may be because of a
                    //    file rewind or wrapping at 2**42. Use new time stamp as first.
                    _tstamp1 = time_stamp;
                    _opt.debug ("evaluateNextStuffing: tstamp1 = " + Decimal (time_stamp.tstamp) + " at " + Decimal (time_stamp.packet));
                }
                else {
                    // Found second time stamp
                    _tstamp2 = time_stamp;
                    _opt.debug ("evaluateNextStuffing: tstamp2 = " + Decimal (time_stamp.tstamp) + " at " + Decimal (time_stamp.packet));
                }
            }
        }

        // If _tstamp2 not set in first segment or after buffer full, we cannot perform bitrate evaluation
        if (!_tstamp2.set() && (initial_position == 0 || !_input.canSeek (initial_position))) {
            std::string msg ("no " + getTimeStampType() + " found");
            if (initial_position > 0) {
                msg += " after packet ";
                msg += Decimal (initial_position);
            }
            if (_opt.reference_pid != PID_NULL) {
                msg += Format (" in PID %d (0x%04X), try another PID or increasing --buffer-size", int (_opt.reference_pid), int (_opt.reference_pid));
            }
            else {
                msg += ", try increasing --buffer-size";
            }
            _opt.fatal (msg);
        }

        // Restore initial position in the file
        if (!_input.seekBackward (size_t (_input.getPacketCount() - initial_position), _opt)) {
            fatalError();
        }

        // If _tstamp2 is not set, we reached the end of file, keep previous settings.
        // Otherwise, compute new settings.
        if (_tstamp2.set()) {
            assert (_tstamp1.set());
            assert (_tstamp1.value().tstamp < _tstamp2.value().tstamp);
            assert (_tstamp1.value().packet < _tstamp2.value().packet);

            // Target number of bits between the two time stamps, plus the previously unstuffed bits
            const PacketCounter target_bits = _additional_bits +
                (PacketCounter (_opt.target_bitrate) * (_tstamp2.value().tstamp - _tstamp1.value().tstamp)) / SYSTEM_CLOCK_FREQ;

            // Target number of packets between the two time stamps
            const PacketCounter target_count = target_bits / (8 * PKT_SIZE);

            // Actual number of packets between the two time stamps in the input file
            const PacketCounter input_count = _tstamp2.value().packet - _tstamp1.value().packet;

            // Compute number of stuffing packets to add
            if (input_count > target_count) {
                _opt.warning ("input bitrate higher than target bitrate, cannot stuff");
                _remaining_stuff_count = 0;
                _current_inter_packet = 0;
                _additional_bits = 0;
            }
            else {
                _remaining_stuff_count = target_count - input_count;
                _current_inter_packet = _remaining_stuff_count / input_count;
                _additional_bits = target_bits % (8 * PKT_SIZE);
            }
        }
    }


    //-------------------------------------------------------------------------
    // Process the content
    //-------------------------------------------------------------------------

    void Stuffer::stuff()
    {
        // Open input file
        if (!_input.open (_opt.input_file, 1, 0, _opt)) {
            fatalError();
        }

        _opt.debug ("input file buffer size: " + Decimal (_input.getBufferSize()) + " packets");

        // Remaining number of bits to stuff representing less than one packet
        _additional_bits = 0;

        // Locate first two time stamps, 
        _tstamp1.reset();
        _tstamp2.reset();
        evaluateNextStuffing();
        assert (_tstamp1.set());
        assert (_tstamp2.set());

        // Create output file
        if (!_output.open (_opt.output_file, false, false, _opt)) {
            fatalError();
        }

        // Write leading stuffing packets
        writeStuffing (_opt.leading_packets);

        // Perform initial stuffing, up to the first time stamp
        simpleInterPacketStuffing (_opt.dyn_initial_inter_packet ? _current_inter_packet : _opt.initial_inter_packet,
                                   _tstamp1.value().packet);

        // Perform stuffing, segment after segment
        while (_tstamp2.set()) {
            assert (_input.getPacketCount() < _tstamp2.value().packet);

            // Perform stuffing on current segment.
            TSPacket pkt;
            while (_input.getPacketCount() < _tstamp2.value().packet && _input.read (&pkt, 1, _opt) == 1) {
                if (!_output.write (&pkt, 1, _opt)) {
                    fatalError();
                }
                const PacketCounter count = std::min (_current_inter_packet, _remaining_stuff_count);
                writeStuffing (count);
                _remaining_stuff_count -= count;
            }
            writeStuffing (_remaining_stuff_count);
            _remaining_stuff_count = 0;

            // Evaluate stuffing need for next segment
            evaluateNextStuffing();
        }

        // Perform final stuffing, up to end of file
        simpleInterPacketStuffing (_opt.dyn_final_inter_packet ? _current_inter_packet : _opt.final_inter_packet,
                                   std::numeric_limits<PacketCounter>::max());

        // Write trailing stuffing packets
        writeStuffing (_opt.leading_packets);

        _opt.verbose ("stuffing completed, read " + Decimal (_input.getPacketCount()) +
                      " packet, written " + Decimal (_output.getPacketCount()) + " packets");

        // Close files
        _output.close (_opt);
        _input.close (_opt);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    Stuffer stuffer (opt);
    stuffer.stuff();
    return EXIT_SUCCESS;
}
