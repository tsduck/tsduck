//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDatagramInputPlugin.h"
#include "tsIPProtocols.h"


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::AbstractDatagramInputPlugin::AbstractDatagramInputPlugin(TSP* tsp_,
                                                             size_t buffer_size,
                                                             const UString& description,
                                                             const UString& syntax,
                                                             const UString& system_time_name,
                                                             const UString& system_time_description,
                                                             TSDatagramInputOptions options) :
    InputPlugin(tsp_, description, syntax),
    _options(options),
    // Ensure at least 7 204-byte packets.
    _inbuf(std::max(buffer_size, 7 * PKT_RS_SIZE)),
    // Resize metadata based on 188-byte packets (max number of packets for that buffer).
    _mdata(_inbuf.size() / PKT_SIZE)
{
    if (bool(_options & TSDatagramInputOptions::REAL_TIME)) {
        option<cn::seconds>(u"display-interval", 'd');
        help(u"display-interval",
             u"Specify the interval in seconds between two displays of the evaluated "
             u"real-time input bitrate. The default is to never display the bitrate. "
             u"This option is ignored if --evaluation-interval is not specified.");

        option<cn::seconds>(u"evaluation-interval", 'e');
        help(u"evaluation-interval",
             u"Specify that the real-time input bitrate shall be evaluated on a regular "
             u"basis. The value specifies the number of seconds between two evaluations. "
             u"By default, the real-time input bitrate is never evaluated and the input "
             u"bitrate is evaluated from the PCR in the input packets.");
    }

    if (bool(_options & TSDatagramInputOptions::ALLOW_RS204)) {
        option(u"rs204");
        help(u"rs204",
             u"Specify that all packets are in 204-byte format. "
             u"By default, the input packet size, 188 or 204 bytes, is automatically detected. "
             u"Use this option only when necessary.");
    }

    // Order of priority for input timestamps.
    _time_priority_enum.add(u"rtp-tsp", TimePriority::RTP_TSP);
    _time_priority_enum.add(u"tsp", TimePriority::TSP_ONLY);
    UString system_help;
    if (!system_time_name.empty()) {
        _default_time_priority = TimePriority::RTP_SYSTEM_TSP;
        _time_priority_enum.add(u"rtp-" + system_time_name + u"-tsp", TimePriority::RTP_SYSTEM_TSP);
        _time_priority_enum.add(system_time_name + u"-rtp-tsp", TimePriority::SYSTEM_RTP_TSP);
        _time_priority_enum.add(system_time_name + u"-tsp", TimePriority::SYSTEM_TSP);
        system_help = u"- " + system_time_name + u" : " + system_time_description + u".\n";
    }

    option(u"timestamp-priority", 0, _time_priority_enum);
    help(u"timestamp-priority", u"name",
         u"Specify how the input timestamp of each packet is computed. "
         u"The name specifies an ordered list. The first available timestamp value is used as input timestamp. "
         u"The possible timestamp sources are:\n"
         u"- rtp : The RTP time stamp, when the UDP packet is an RTP packet.\n" +
         system_help +
         u"- tsp : A software timestamp, provided by tsp when the input plugin returns a chunk of packets.\n"
         u"The tsp-provided timestamp is always available, always comes last and is less precise. "
         u"The default is " + _time_priority_enum.name(_default_time_priority) + u".");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::AbstractDatagramInputPlugin::isRealTime()
{
    return bool(_options & TSDatagramInputOptions::REAL_TIME);
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::AbstractDatagramInputPlugin::getOptions()
{
    if (bool(_options & TSDatagramInputOptions::REAL_TIME)) {
        getChronoValue(_eval_time, u"evaluation-interval");
        getChronoValue(_display_time, u"display-interval");
    }
    _rs204_format = bool(_options & TSDatagramInputOptions::ALLOW_RS204) && present(u"rs204");
    getIntValue(_time_priority, u"timestamp-priority", _default_time_priority);
    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::AbstractDatagramInputPlugin::start()
{
    // Initialize working data.
    _inbuf_count = _inbuf_next = _mdata_next = 0;
    _start = _start_0 = _start_1 = _next_display = Time::Epoch;
    _packets = _packets_0 = _packets_1 = 0;

    // Expected packet size. Zero means any.
    if (_rs204_format) {
        _packet_size = PKT_RS_SIZE;
    }
    else if (!(_options & TSDatagramInputOptions::ALLOW_RS204)) {
        _packet_size = PKT_SIZE;
    }
    else {
        _packet_size = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Input bitrate evaluation method
//----------------------------------------------------------------------------

ts::BitRate ts::AbstractDatagramInputPlugin::getBitrate()
{
    if (!(_options & TSDatagramInputOptions::REAL_TIME) || _eval_time <= cn::milliseconds::zero() || _start_0 == _start_1) {
        // Input bitrate not evaluated at all or first evaluation period not yet complete
        return 0;
    }
    else {
        // Evaluate bitrate since start of previous evaluation period.
        // The current period may be too short for correct evaluation.
        return PacketBitRate(_packets_0, Time::CurrentUTC() - _start_0);
    }
}

ts::BitRateConfidence ts::AbstractDatagramInputPlugin::getBitrateConfidence()
{
    // The returned bitrate is based on the system clock.
    return BitRateConfidence::CLOCK;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::AbstractDatagramInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    cn::microseconds timestamp = cn::microseconds(-1);
    TimeSource timesource = TimeSource::UNDEFINED;

    // Check if we receive new packets or process remain of previous buffer.
    bool new_packets = false;

    // If there is no remaining packet in the input buffer, wait for a datagram message.
    // Loop until we get some TS packets.
    while (_inbuf_count == 0) {

        // Wait for a datagram message
        size_t insize = 0;
        if (!receiveDatagram(_inbuf.data(), _inbuf.size(), insize, timestamp, timesource)) {
            return 0;
        }

        // Look for TS packets in the UDP message.
        new_packets = TSPacket::Locate(_inbuf.data(), insize, _inbuf_next, _inbuf_count, _packet_size);

        if (new_packets) {
            assert(_packet_size == PKT_SIZE || _packet_size == PKT_RS_SIZE);

            // Look for an RTP header before the first packet. There is no clear proof of the presence of the RTP header.
            // We check if the header size is large enough for an RTP header and if the "RTP payload type" is MPEG-2 TS.
            const bool rtp = _inbuf_next >= RTP_HEADER_SIZE && (_inbuf[1] & 0x7F) == RTP_PT_MP2T;
            const ts::rtp_units rtp_timestamp = ts::rtp_units(rtp ? GetUInt32(_inbuf.data() + 4) : 0);

            // Use RTP time stamp if there is one and RTP is the preferred choice.
            bool use_rtp = false;
            bool use_kernel = false;
            switch (_time_priority) {
                case RTP_SYSTEM_TSP:
                    use_rtp = rtp;
                    use_kernel = !rtp && timestamp >= cn::microseconds::zero();
                    break;
                case SYSTEM_RTP_TSP:
                    use_kernel = timestamp >= cn::microseconds::zero();
                    use_rtp = !use_kernel && rtp;
                    break;
                case RTP_TSP:
                    use_rtp = rtp;
                    use_kernel = false;
                    break;
                case SYSTEM_TSP:
                    use_kernel = timestamp >= cn::microseconds::zero();
                    use_rtp = false;
                    break;
                case TSP_ONLY:
                default:
                    use_rtp = false;
                    use_kernel = false;
                    break;
            }

            // Build time stamps in packet metadata.
            _mdata_next = 0;
            for (size_t i = 0; i < _inbuf_count; ++i) {
                TSPacketMetadata& md(_mdata[i]);
                md.reset();
                md.setDatagram(_datagram);
                if (use_rtp) {
                    md.setInputTimeStamp(rtp_timestamp, TimeSource::RTP);
                }
                else if (use_kernel) {
                    md.setInputTimeStamp(timestamp, timesource);
                }
                // Copy 204-byte trailer in metadata.
                if (_packet_size == PKT_RS_SIZE) {
                    md.setAuxData(_inbuf.data() + _inbuf_next + i * PKT_RS_SIZE + PKT_SIZE, RS_SIZE);
                }
            }

            break; // found packets.
        }

        // No TS packet found in UDP message, wait for another one.
        debug(u"no TS packet in message, %s bytes", insize);
    }

    // If new packets were received, we may need to re-evaluate the real-time input bitrate.
    if (new_packets && bool(_options & TSDatagramInputOptions::REAL_TIME) && _eval_time > cn::milliseconds::zero()) {

        const Time now(Time::CurrentUTC());

        // Detect start time
        if (_packets == 0) {
            _start = _start_0 = _start_1 = now;
            if (_display_time > cn::milliseconds::zero()) {
                _next_display = now + _display_time;
            }
        }

        // Count packets
        _packets += _inbuf_count;
        _packets_0 += _inbuf_count;
        _packets_1 += _inbuf_count;

        // Detect new evaluation period
        if (now >= _start_1 + _eval_time) {
            _start_0 = _start_1;
            _packets_0 = _packets_1;
            _start_1 = now;
            _packets_1 = 0;

        }

        // Check if evaluated bitrate should be displayed
        if (_display_time > cn::milliseconds::zero() && now >= _next_display) {
            _next_display += _display_time;
            const cn::milliseconds ms_current = Time::CurrentUTC() - _start_0;
            const cn::milliseconds ms_total = Time::CurrentUTC() - _start;
            const BitRate br_current = PacketBitRate(_packets_0, ms_current);
            const BitRate br_average = PacketBitRate(_packets, ms_total);
            info(u"input bitrate: %s, average: %s",
                 br_current == 0 ? u"undefined" : br_current.toString() + u" b/s",
                 br_average == 0 ? u"undefined" : br_average.toString() + u" b/s");
        }
    }

    // Return packets from the input buffer
    size_t pkt_cnt = std::min(_inbuf_count, max_packets);
    TSPacket::Copy(buffer, _inbuf.data() + _inbuf_next, pkt_cnt, _packet_size);
    TSPacketMetadata::Copy(pkt_data, &_mdata[_mdata_next], pkt_cnt);
    _inbuf_count -= pkt_cnt;
    _inbuf_next += pkt_cnt * _packet_size;
    _mdata_next += pkt_cnt;

    return pkt_cnt;
}
