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

#include "tsAbstractDatagramInputPlugin.h"
#include "tsSysUtils.h"
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
                                                             bool real_time) :
    InputPlugin(tsp_, description, syntax),
    _real_time(real_time),
    _eval_time(0),
    _display_time(0),
    _time_priority_enum(),
    _time_priority(RTP_TSP),
    _default_time_priority(RTP_TSP),
    _next_display(Time::Epoch),
    _start(Time::Epoch),
    _packets(0),
    _start_0(Time::Epoch),
    _packets_0(0),
    _start_1(Time::Epoch),
    _packets_1(0),
    _inbuf_count(0),
    _inbuf_next(0),
    _mdata_next(0),
    _inbuf(std::max(buffer_size, 7 * PKT_SIZE)),
    _mdata(_inbuf.size() / PKT_SIZE)
{
    if (_real_time) {
        option(u"display-interval", 'd', POSITIVE);
        help(u"display-interval",
             u"Specify the interval in seconds between two displays of the evaluated "
             u"real-time input bitrate. The default is to never display the bitrate. "
             u"This option is ignored if --evaluation-interval is not specified.");

        option(u"evaluation-interval", 'e', POSITIVE);
        help(u"evaluation-interval",
             u"Specify that the real-time input bitrate shall be evaluated on a regular "
             u"basis. The value specifies the number of seconds between two evaluations. "
             u"By default, the real-time input bitrate is never evaluated and the input "
             u"bitrate is evaluated from the PCR in the input packets.");
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
         u"Specify how the input time-stamp of each packet is computed. "
         u"The name specifies an ordered list. The first available time-stamp value is used as input time-stamp. "
         u"The possible time-stamp sources are:\n"
         u"- rtp : The RTP time stamp, when the UDP packet is an RTP packet.\n" +
         system_help +
         u"- tsp : A software time-stamp, provided by tsp when the input plugin returns a chunk of packets.\n"
         u"The tsp-provided time-stamp is always available, always comes last and is less precise. "
         u"The default is " + _time_priority_enum.name(_default_time_priority) + u".");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::AbstractDatagramInputPlugin::isRealTime()
{
    return _real_time;
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::AbstractDatagramInputPlugin::getOptions()
{
    if (_real_time) {
        _eval_time = MilliSecPerSec * intValue<MilliSecond>(u"evaluation-interval", 0);
        _display_time = MilliSecPerSec * intValue<MilliSecond>(u"display-interval", 0);
    }
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
    return true;
}


//----------------------------------------------------------------------------
// Input bitrate evaluation method
//----------------------------------------------------------------------------

ts::BitRate ts::AbstractDatagramInputPlugin::getBitrate()
{
    if (!_real_time || _eval_time <= 0 || _start_0 == _start_1) {
        // Input bitrate not evaluated at all or first evaluation period not yet complete
        return 0;
    }
    else {
        // Evaluate bitrate since start of previous evaluation period.
        // The current period may be too short for correct evaluation.
        const MilliSecond ms = Time::CurrentUTC() - _start_0;
        return ms == 0 ? 0 : BitRate(_packets_0 * PKT_SIZE_BITS * MilliSecPerSec) / ms;
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
    MicroSecond timestamp = -1;

    // Check if we receive new packets or process remain of previous buffer.
    bool new_packets = false;

    // If there is no remaining packet in the input buffer, wait for a datagram message.
    // Loop until we get some TS packets.
    while (_inbuf_count == 0) {

        // Wait for a datagram message
        size_t insize = 0;
        if (!receiveDatagram(_inbuf.data(), _inbuf.size(), insize, timestamp)) {
            return 0;
        }

        // Look for TS packets in the UDP message.
        new_packets = TSPacket::Locate(_inbuf.data(), insize, _inbuf_next, _inbuf_count);

        if (new_packets) {

            // Look for an RTP header before the first packet. There is no clear proof of the presence of the RTP header.
            // We check if the header size is large enough for an RTP header and if the "RTP payload type" is MPEG-2 TS.
            const bool rtp = _inbuf_next >= RTP_HEADER_SIZE && (_inbuf[1] & 0x7F) == RTP_PT_MP2T;
            const uint32_t rtp_timestamp = rtp ? GetUInt32(_inbuf.data() + 4) : 0;

            // Use RTP time stamp if there is one and RTP is the preferred choice.
            bool use_rtp = false;
            bool use_kernel = false;
            switch (_time_priority) {
                case RTP_SYSTEM_TSP:
                    use_rtp = rtp;
                    use_kernel = !rtp && timestamp >= 0;
                    break;
                case SYSTEM_RTP_TSP:
                    use_kernel = timestamp >= 0;
                    use_rtp = !use_kernel && rtp;
                    break;
                case RTP_TSP:
                    use_rtp = rtp;
                    use_kernel = false;
                    break;
                case SYSTEM_TSP:
                    use_kernel = timestamp >= 0;
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
                if (use_rtp) {
                    // RTP time stamp unit is 90 kHz (RTP_RATE_MP2T)
                    _mdata[i].setInputTimeStamp(rtp_timestamp, RTP_RATE_MP2T, TimeSource::RTP);
                }
                else if (use_kernel) {
                    // IP time stamp unit is microseconds.
                    _mdata[i].setInputTimeStamp(uint64_t(timestamp), MicroSecPerSec, TimeSource::KERNEL);
                }
                else {
                    _mdata[i].clearInputTimeStamp();
                }
            }

            break; // found packets.
        }

        // No TS packet found in UDP message, wait for another one.
        tsp->debug(u"no TS packet in message, %s bytes", {insize});
    }

    // If new packets were received, we may need to re-evaluate the real-time input bitrate.
    if (new_packets && _real_time && _eval_time > 0) {

        const Time now(Time::CurrentUTC());

        // Detect start time
        if (_packets == 0) {
            _start = _start_0 = _start_1 = now;
            if (_display_time > 0) {
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
        if (_display_time > 0 && now >= _next_display) {
            _next_display += _display_time;
            const MilliSecond ms_current = Time::CurrentUTC() - _start_0;
            const MilliSecond ms_total = Time::CurrentUTC() - _start;
            const BitRate br_current = ms_current == 0 ? 0 : BitRate(_packets_0 * PKT_SIZE_BITS * MilliSecPerSec) / ms_current;
            const BitRate br_average = ms_total == 0 ? 0 : BitRate(_packets * PKT_SIZE_BITS * MilliSecPerSec) / ms_total;
            tsp->info(u"input bitrate: %s, average: %s", {
                br_current == 0 ? u"undefined" : br_current.toString() + u" b/s",
                br_average == 0 ? u"undefined" : br_average.toString() + u" b/s"});
        }
    }

    // Return packets from the input buffer
    size_t pkt_cnt = std::min(_inbuf_count, max_packets);
    TSPacket::Copy(buffer, _inbuf.data() + _inbuf_next, pkt_cnt);
    TSPacketMetadata::Copy(pkt_data, &_mdata[_mdata_next], pkt_cnt);
    _inbuf_count -= pkt_cnt;
    _inbuf_next += pkt_cnt * PKT_SIZE;
    _mdata_next += pkt_cnt;

    return pkt_cnt;
}
