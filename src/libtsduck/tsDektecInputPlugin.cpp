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
//! @file tsDektecInputPlugin.h
//!
//! Declare the ts::DektecInputPlugin class.
//!
//----------------------------------------------------------------------------

#include "tsDektecInputPlugin.h"
#include "tsDektecUtils.h"
#include "tsDektecDevice.h"
#include "tsDektecVPD.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsIntegerUtils.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Stubs when compiled without Dektec support.
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

ts::DektecInputPlugin::DektecInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, "Receive packets from a Dektec DVB-ASI device.", "[options]"),
    _guts(0)
{
    setHelp(TS_NO_DTAPI_MESSAGE "\n");
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
    InputPlugin(tsp_, "Receive packets from a Dektec DVB-ASI device.", "[options]"),
    _guts(new Guts)
{
    CheckNonNull(_guts);

    option("channel", 'c', UNSIGNED);
    option("device", 'd', UNSIGNED);

    setHelp("Options:\n"
            "\n"
            "  -c value\n"
            "  --channel value\n"
            "      Channel index on the input Dektec device. By default, use the\n"
            "      first input channel on the device.\n"
            "\n"
            "  -d value\n"
            "  --device value\n"
            "      Device index, from 0 to N-1 (with N being the number of Dektec devices\n"
            "      in the system). Use the command \"tsdektec -a [-v]\" to have a\n"
            "      complete list of devices in the system. By default, use the first\n"
            "      input Dektec device.\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::start()
{
    if (_guts->is_started) {
        tsp->error ("already started");
        return false;
    }

    // Get command line arguments
    _guts->dev_index = intValue<int>("device", -1);
    _guts->chan_index = intValue<int>("channel", -1);

    // Locate the device
    if (!_guts->device.getDevice(_guts->dev_index, _guts->chan_index, true, *tsp)) {
        return false;
    }

    // Open the device
    Dtapi::DTAPI_RESULT status = _guts->dtdev.AttachToSerial(_guts->device.desc.m_Serial);
    if (status != DTAPI_OK) {
        tsp->error(Format("error attaching input Dektec device %d: ", _guts->dev_index) + DektecStrError(status));
        return false;
    }

    // Open the input channel
    status = _guts->chan.AttachToPort(&_guts->dtdev, _guts->device.input[_guts->chan_index].m_Port);
    if (status != DTAPI_OK) {
        tsp->error(Format("error attaching input channel %d of Dektec device %d: ", _guts->chan_index, _guts->dev_index) + DektecStrError(status));
        _guts->dtdev.Detach();
        return false;
    }

    // Reset input channel
    status = _guts->chan.Reset(DTAPI_FULL_RESET);
    if (status != DTAPI_OK) {
        tsp->error("input device reset error: " + DektecStrError(status));
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Set the receiving packet size to 188 bytes (the size of the packets
    // which are returned by the board to the application, dropping extra 16
    // bytes if the transmitted packets are 204-byte).
    status = _guts->chan.SetRxMode(DTAPI_RXMODE_ST188);
    if (status != DTAPI_OK) {
        tsp->error("device SetRxMode error: " + DektecStrError(status));
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Start the capture on the input device (set receive control to "receive")
    status = _guts->chan.SetRxControl(DTAPI_RXCTRL_RCV);
    if (status != DTAPI_OK) {
        tsp->error("device SetRxControl error: " + DektecStrError(status));
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
        tsp->error("error getting Dektec device input bitrate: " + DektecStrError(status));
        return 0;
    }
    if (_guts->got_bitrate && bitrate != int(_guts->cur_bitrate)) {
        tsp->verbose("new input bitrate: " + Decimal(bitrate) + " b/s");
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
            tsp->error("error getting input fifo load: " + DektecStrError(status));
        }
        if (fifo_load >= int(DTA_FIFO_SIZE)) {
            // Input overflow.
            tsp->warning("input fifo full, possible packet loss");
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
        tsp->error(Format("capture error on Dektec device %d: ", _guts->dev_index) + DektecStrError(status));
        return 0;
    }
}

#endif // TS_NO_DTAPI
