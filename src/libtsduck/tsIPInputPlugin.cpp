//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsIPInputPlugin.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::IPInputPlugin::IPInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive TS packets from UDP/IP, multicast or unicast", u"[options] [address:]port"),
    _sock(*tsp_),
    _eval_time(0),
    _display_time(0),
    _next_display(Time::Epoch),
    _start(Time::Epoch),
    _packets(0),
    _start_0(Time::Epoch),
    _packets_0(0),
    _start_1(Time::Epoch),
    _packets_1(0),
    _inbuf_count(0),
    _inbuf_next(0),
    _inbuf()
{
    // Add UDP receiver common options.
    _sock.defineOptions(*this);

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


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::getOptions()
{
    // Get command line arguments
    _eval_time = MilliSecPerSec * intValue<MilliSecond>(u"evaluation-interval", 0);
    _display_time = MilliSecPerSec * intValue<MilliSecond>(u"display-interval", 0);
    return _sock.load(*this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::start()
{
    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    // Socket now ready.
    // Initialize working data.
    _inbuf_count = _inbuf_next = 0;
    _start = _start_0 = _start_1 = _next_display = Time::Epoch;
    _packets = _packets_0 = _packets_1 = 0;

    return true;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::stop()
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::abortInput()
{
    _sock.close(*tsp);
    return true;
}

//----------------------------------------------------------------------------
// Input bitrate evaluation method
//----------------------------------------------------------------------------

ts::BitRate ts::IPInputPlugin::getBitrate()
{
    if (_eval_time <= 0 || _start_0 == _start_1) {
        // Input bitrate not evaluated at all or first evaluation period not yet complete
        return 0;
    }
    else {
        // Evaluate bitrate since start of previous evaluation period.
        // The current period may be too short for correct evaluation.
        const MilliSecond ms = Time::CurrentUTC() - _start_0;
        return ms == 0 ? 0 : BitRate((_packets_0 * PKT_SIZE * 8 * MilliSecPerSec) / ms);
    }
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::IPInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    // Check if we receive new packets or process remain of previous buffer.
    bool new_packets = false;

    // If there is no remaining packet in the input buffer, wait for a UDP
    // message. Loop until we get some TS packets.
    while (_inbuf_count <= 0) {

        // Wait for a UDP message
        SocketAddress sender;
        SocketAddress destination;
        size_t insize;
        if (!_sock.receive(_inbuf, sizeof(_inbuf), insize, sender, destination, tsp, *tsp)) {
            return 0;
        }

        // Locate the TS packets inside the UDP message. Basically, we
        // expect the message to contain only TS packets. However, we
        // will face the following situations:
        // - Presence of a header preceeding the first TS packet (typically
        //   when the TS packets are encapsulated in RTP).
        // - Presence of a truncated packet at the end of message.

        // To face the first situation, we look backward from the end of
        // the message, looking for a 0x47 sync byte every 188 bytes, going
        // backward.

        const uint8_t* p;
        for (p = _inbuf + insize; p >= _inbuf + PKT_SIZE && p[-int(PKT_SIZE)] == SYNC_BYTE; p -= PKT_SIZE) {}

        if (p < _inbuf + insize) {
            // Some packets were found
            _inbuf_next = p - _inbuf;
            _inbuf_count = (_inbuf + insize - p) / PKT_SIZE;
            new_packets = true;
            break; // exit receive loop
        }

        // If no TS packet is found using the first method, we restart from
        // the beginning of the message, looking for a 0x47 sync byte every
        // 188 bytes, going forward. If we find this pattern, followed by
        // less than 188 bytes, then we have found a sequence of TS packets.

        const uint8_t* max = _inbuf + insize - PKT_SIZE; // max address for a TS packet
        _inbuf_count = 0;

        for (p = _inbuf; p <= max; p++) {
            if (*p == SYNC_BYTE) {
                // Verify that we get a 0x47 sync byte every 188 bytes up
                // to the end of message (not leaving more than one truncated
                // TS packet at the end of the message).
                const uint8_t* end;
                for (end = p; end <= max && *end == SYNC_BYTE; end += PKT_SIZE) {}
                if (end > max) {
                    // Less than 188 bytes after last packet. Consider we are OK
                    _inbuf_next = p - _inbuf;
                    _inbuf_count = (end - p) / PKT_SIZE;
                    new_packets = true;
                    break; // exit packet search loop
                }
            }
        }

        if (new_packets) {
            break; // exit receive loop
        }

        // No TS packet found in UDP message, wait for another one.
        tsp->debug(u"no TS packet in message from %s, %s bytes", {sender, insize});
    }

    // If new packets were received, we may need to re-evaluate the real-time input bitrate.
    if (new_packets && _eval_time > 0) {

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
            const BitRate br_current = ms_current == 0 ? 0 : BitRate((_packets_0 * PKT_SIZE * 8 * MilliSecPerSec) / ms_current);
            const BitRate br_average = ms_total == 0 ? 0 : BitRate((_packets * PKT_SIZE * 8 * MilliSecPerSec) / ms_total);
            tsp->info(u"IP input bitrate: %s, average: %s", {
                br_current == 0 ? u"undefined" : UString::Decimal(br_current) + u" b/s",
                br_average == 0 ? u"undefined" : UString::Decimal(br_average) + u" b/s"});
        }
    }

    // Return packets from the input buffer
    size_t pkt_cnt = std::min(_inbuf_count, max_packets);
    TSPacket::Copy(buffer, _inbuf + _inbuf_next, pkt_cnt);
    _inbuf_count -= pkt_cnt;
    _inbuf_next += pkt_cnt * PKT_SIZE;

    return pkt_cnt;
}
