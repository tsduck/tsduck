//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsDektecInputPlugin.h"
#include "tsDektecUtils.h"
#include "tsDektecDevice.h"
#include "tsDektecVPD.h"
#include "tsIntegerUtils.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Stubs when compiled without Dektec support.
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

ts::DektecInputPlugin::DektecInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive packets from a Dektec DVB-ASI device", u"[options]"),
    _guts(0)
{
    setHelp(TS_NO_DTAPI_MESSAGE u"\n");
}

ts::DektecInputPlugin::~DektecInputPlugin()
{
}

bool ts::DektecInputPlugin::start()
{
    tsp->error(TS_NO_DTAPI_MESSAGE);
    return false;
}

bool ts::DektecInputPlugin::stop()
{
    return true;
}

ts::BitRate ts::DektecInputPlugin::getBitrate()
{
    return 0;
}

size_t ts::DektecInputPlugin::receive(TSPacket* buffer, size_t max_packets)
{
    tsp->error(TS_NO_DTAPI_MESSAGE);
    return 0;
}

#else

//----------------------------------------------------------------------------
// Class internals.
//----------------------------------------------------------------------------

class ts::DektecInputPlugin::Guts
{
public:
    bool                is_started;   // Device started
    int                 dev_index;    // Dektec device index
    int                 chan_index;   // Device input channel index
    DektecDevice        device;       // Device characteristics
    Dtapi::DtDevice     dtdev;        // Device descriptor
    Dtapi::DtInpChannel chan;         // Input channel
    int                 init_cnt;     // Count the first inputs
    BitRate             cur_bitrate;  // Current input bitrate
    bool                got_bitrate;  // Got bitrate at least once.

    Guts() :                          // Constructor.
        is_started(false),
        dev_index(-1),
        chan_index(-1),
        device(),
        dtdev(),
        chan(),
        init_cnt(0),
        cur_bitrate(0),
        got_bitrate(false)
    {
    }
};


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::DektecInputPlugin::DektecInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive packets from a Dektec DVB-ASI device.", u"[options]"),
    _guts(new Guts)
{
    CheckNonNull(_guts);

    option(u"channel", 'c', UNSIGNED);
    option(u"device", 'd', UNSIGNED);

    setHelp(u"Options:\n"
            u"\n"
            u"  -c value\n"
            u"  --channel value\n"
            u"      Channel index on the input Dektec device. By default, use the\n"
            u"      first input channel on the device.\n"
            u"\n"
            u"  -d value\n"
            u"  --device value\n"
            u"      Device index, from 0 to N-1 (with N being the number of Dektec devices\n"
            u"      in the system). Use the command \"tsdektec -a [-v]\" to have a\n"
            u"      complete list of devices in the system. By default, use the first\n"
            u"      input Dektec device.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::start()
{
    if (_guts->is_started) {
        tsp->error(u"already started");
        return false;
    }

    // Get command line argumentsu
    _guts->dev_index = intValue<int>(u"device", -1);
    _guts->chan_index = intValue<int>(u"channel", -1);

    // Locate the device
    if (!_guts->device.getDevice(_guts->dev_index, _guts->chan_index, true, *tsp)) {
        return false;
    }

    // Open the device
    Dtapi::DTAPI_RESULT status = _guts->dtdev.AttachToSerial(_guts->device.desc.m_Serial);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching input Dektec device %d: %s", {_guts->dev_index, DektecStrError(status)});
        return false;
    }

    // Open the input channel
    status = _guts->chan.AttachToPort(&_guts->dtdev, _guts->device.input[_guts->chan_index].m_Port);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching input channel %d of Dektec device %d: %s", {_guts->chan_index, _guts->dev_index, DektecStrError(status)});
        _guts->dtdev.Detach();
        return false;
    }

    // Reset input channel
    status = _guts->chan.Reset(DTAPI_FULL_RESET);
    if (status != DTAPI_OK) {
        tsp->error(u"input device reset error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Set the receiving packet size to 188 bytes (the size of the packets
    // which are returned by the board to the application, dropping extra 16
    // bytes if the transmitted packets are 204-byte).
    status = _guts->chan.SetRxMode(DTAPI_RXMODE_ST188);
    if (status != DTAPI_OK) {
        tsp->error(u"device SetRxMode error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Start the capture on the input device (set receive control to "receive")
    status = _guts->chan.SetRxControl(DTAPI_RXCTRL_RCV);
    if (status != DTAPI_OK) {
        tsp->error(u"device SetRxControl error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Consider that the first 5 inputs are "initialization". If a full input
    // fifo is observed here, ignore it. Later, a full fifo indicates potential
    // packet loss.
    _guts->init_cnt = 5;
    _guts->is_started = true;
    return true;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::stop()
{
    if (_guts->is_started) {
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        _guts->is_started = false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Input destructor
//----------------------------------------------------------------------------

ts::DektecInputPlugin::~DektecInputPlugin()
{
    if (_guts != 0) {
        stop();
        delete _guts;
        _guts = 0;
    }
}


//----------------------------------------------------------------------------
// Get input bitrate
//----------------------------------------------------------------------------

ts::BitRate ts::DektecInputPlugin::getBitrate()
{
    if (!_guts->is_started) {
        return 0;
    }

    int bitrate;
    Dtapi::DTAPI_RESULT status = _guts->chan.GetTsRateBps(bitrate);

    if (status != DTAPI_OK) {
        tsp->error(u"error getting Dektec device input bitrate: " + DektecStrError(status));
        return 0;
    }
    if (_guts->got_bitrate && bitrate != int(_guts->cur_bitrate)) {
        tsp->verbose(u"new input bitrate: %'d b/s", {bitrate});
    }

    _guts->got_bitrate = true;
    return _guts->cur_bitrate = bitrate;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::DektecInputPlugin::receive(TSPacket* buffer, size_t max_packets)
{
    if (!_guts->is_started) {
        return 0;
    }

    Dtapi::DTAPI_RESULT status;

    // After initialization, we check the receive FIFO load before reading it.
    // If the FIFO is full, we have lost packets.
    if (_guts->init_cnt > 0) {
        _guts->init_cnt--;
    }
    if (_guts->init_cnt == 0) {
        int fifo_load;
        status = _guts->chan.GetFifoLoad(fifo_load);
        if (status != DTAPI_OK) {
            tsp->error(u"error getting input fifo load: %s", {DektecStrError(status)});
        }
        if (fifo_load >= int(DTA_FIFO_SIZE)) {
            // Input overflow.
            tsp->warning(u"input fifo full, possible packet loss");
        }
    }

    // Do not read more than what a DTA device accepts
    size_t size = RoundDown(std::min(max_packets * PKT_SIZE, DTA_MAX_IO_SIZE), PKT_SIZE);

    // Receive packets (wait if no input signal)
    status = _guts->chan.Read(reinterpret_cast<char*> (buffer), int(size));
    if (status == DTAPI_OK) {
        return size / PKT_SIZE;
    }
    else {
        tsp->error(u"capture error on Dektec device %d: %s", {_guts->dev_index, DektecStrError(status)});
        return 0;
    }
}

#endif // TS_NO_DTAPI
