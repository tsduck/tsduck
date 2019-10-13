//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2019, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsTunerArgs.h"
#include "tsTuner.h"
#include "tsChannelFile.h"
#include "tsDuckContext.h"
#include "tsHFBand.h"
#include "tsArgs.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TunerArgs::TunerArgs(bool info_only, bool allow_short_options) :
    ModulationArgs(allow_short_options),
    device_name(),
    signal_timeout(Tuner::DEFAULT_SIGNAL_TIMEOUT),
    receive_timeout(0),
#if defined(TS_LINUX)
    demux_buffer_size(Tuner::DEFAULT_DEMUX_BUFFER_SIZE),
#elif defined(TS_WINDOWS)
    demux_queue_size(Tuner::DEFAULT_SINK_QUEUE_SIZE),
#endif
    channel_name(),
    tuning_file_name(),
    _info_only(info_only)
{
}


//----------------------------------------------------------------------------
// Reset all values, they become "unset"
//----------------------------------------------------------------------------

void ts::TunerArgs::reset()
{
    device_name.clear();
    signal_timeout = Tuner::DEFAULT_SIGNAL_TIMEOUT;
    receive_timeout = 0;
#if defined(TS_LINUX)
    demux_buffer_size = Tuner::DEFAULT_DEMUX_BUFFER_SIZE;
#elif defined(TS_WINDOWS)
    demux_queue_size = Tuner::DEFAULT_SINK_QUEUE_SIZE;
#endif
    channel_name.clear();
    tuning_file_name.clear();

    // Reset superclass.
    ModulationArgs::reset();
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TunerArgs::loadArgs(DuckContext& duck, Args& args)
{
    bool status = true;
    reset();

    // Tuner identification.
    if (args.present(u"adapter") && args.present(u"device-name")) {
        args.error(u"choose either --adapter or --device-name but not both");
        status = false;
    }
    if (args.present(u"device-name")) {
        device_name = args.value(u"device-name");
    }
    else if (args.present(u"adapter")) {
        const int adapter = args.intValue(u"adapter", 0);
#if defined(TS_LINUX)
        device_name.format(u"/dev/dvb/adapter%d", {adapter});
#elif defined(TS_WINDOWS)
        device_name.format(u":%d", {adapter});
#else
        // Does not mean anything, just for error messages.
        device_name.format(u"DVB adapter %d", {adapter});
#endif
    }

    // Tuning options.
    if (!_info_only) {
        // Reception parameters.
        signal_timeout = args.intValue<MilliSecond>(u"signal-timeout", Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) * 1000;
        receive_timeout = args.intValue<MilliSecond>(u"receive-timeout", 0);
#if defined(TS_LINUX)
        demux_buffer_size = args.intValue<size_t>(u"demux-buffer-size", Tuner::DEFAULT_DEMUX_BUFFER_SIZE);
#elif defined(TS_WINDOWS)
        demux_queue_size = args.intValue<size_t>(u"demux-queue-size", Tuner::DEFAULT_SINK_QUEUE_SIZE);
#endif

        // Tuning parameters from superclass.
        status = ModulationArgs::loadArgs(duck, args) && status;

        // Locating the transponder by channel
        channel_name = args.value(u"channel-transponder");
        tuning_file_name = args.value(u"tuning-file");

        // Mutually exclusive methods of locating the channels
        if (!channel_name.empty() && hasModulationArgs()) {
            args.error(u"--channel-transponder and individual tuning options are incompatible");
            status = false;
        }
    }

    return status;
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TunerArgs::defineArgs(Args& args) const
{
    // Tuner identification.
    args.option(u"adapter", allowShortOptions() ? u'a' : 0, Args::UNSIGNED);
    args.help(u"adapter", u"N",
#if defined(TS_LINUX)
              u"Specifies the Linux DVB adapter N (/dev/dvb/adapterN). "
#elif defined(TS_WINDOWS)
              u"Specifies the Nth DVB adapter in the system. "
#endif
              u"This option can be used instead of device name.");

    args.option(u"device-name", allowShortOptions() ? u'd' : 0, Args::STRING);
    args.help(u"device-name", u"name",
#if defined(TS_LINUX)
              u"Specify the DVB receiver device name, /dev/dvb/adapterA[:F[:M[:V]]] "
              u"where A = adapter number, F = frontend number (default: 0), M = demux "
              u"number (default: 0), V = dvr number (default: 0). "
#elif defined(TS_WINDOWS)
              u"Specify the receiver device name. This is a DirectShow/BDA tuner "
              u"filter name (not case sensitive, blanks are ignored). "
#endif
              u"By default, the first receiver device is used. "
              u"Use the tslsdvb utility to list all DVB devices. ");

    // All other parameters are used to control the tuner.
    if (!_info_only) {

        // Reception parameters.
        args.option(u"receive-timeout", 0, Args::UNSIGNED);
        args.help(u"receive-timeout", u"milliseconds",
                  u"Specifies the timeout, in milliseconds, for each receive operation. "
                  u"To disable the timeout and wait indefinitely for packets, specify zero. "
                  u"This is the default.");

        args.option(u"signal-timeout", 0, Args::UNSIGNED);
        args.help(u"signal-timeout", u"seconds",
                  u"Specifies the timeout, in seconds, for DVB signal locking. If no signal "
                  u"is detected after this timeout, the command aborts. To disable the "
                  u"timeout and wait indefinitely for the signal, specify zero. The default "
                  u"is " + UString::Decimal(Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) + u" seconds.");

#if defined(TS_LINUX)

        args.option(u"demux-buffer-size", 0, Args::UNSIGNED);
        args.help(u"demux-buffer-size",
                  u"Default buffer size, in bytes, of the demux device. "
                  u"The default is 1 MB.");

#elif defined(TS_WINDOWS)

        args.option(u"demux-queue-size", 0, Args::UNSIGNED);
        args.help(u"demux-queue-size",
                  u"Specify the maximum number of media samples in the queue between the "
                  u"DirectShow capture thread and the input plugin thread. The default is " +
                  UString::Decimal(Tuner::DEFAULT_SINK_QUEUE_SIZE) + u" media samples.");

#endif

        // Tuning options from superclass.
        ModulationArgs::defineArgs(args);

        // Tuning using a channel configuration file.
        args.option(u"channel-transponder", allowShortOptions() ? 'c' : 0, Args::STRING);
        args.help(u"channel-transponder", u"name",
                  u"Tune to the transponder containing the specified channel. The channel name "
                  u"is not case-sensitive and blanks are ignored. The channel is searched in a "
                  u"\"tuning file\" and the corresponding tuning information in this file is used.");

        args.option(u"tuning-file", 0, Args::STRING);
        args.help(u"tuning-file",
                  u"Tuning configuration file to use for option -c or --channel-transponder. "
                  u"This is an XML file. See the TSDuck user's guide for more details. "
                  u"Tuning configuration files can be created using the tsscan utility or the nitscan plugin. "
                  u"The location of the default tuning configuration file depends on the system."
#if defined(TS_LINUX)
                  u" On Linux, the default file is $HOME/.tsduck.channels.xml."
#elif defined(TS_WINDOWS)
                  u" On Windows, the default file is %APPDATA%\\tsduck\\channels.xml."
#endif
                  );
    }
}


//----------------------------------------------------------------------------
// Open a tuner and configure it according to the parameters in this object.
//----------------------------------------------------------------------------

bool ts::TunerArgs::configureTuner(Tuner& tuner, Report& report) const
{
    if (tuner.isOpen()) {
        report.error(u"tuner is already open");
        return false;
    }

    // Open DVB tuner. Use first device by default (if device name is empty).
    if (!tuner.open(device_name, _info_only, report)) {
        return false;
    }

    // Set configuration parameters.
    tuner.setSignalTimeout(signal_timeout);
    if (!tuner.setReceiveTimeout(receive_timeout, report)) {
        tuner.close(NULLREP);
        return false;
    }

#if defined(TS_LINUX)
    tuner.setSignalPoll(Tuner::DEFAULT_SIGNAL_POLL);
    tuner.setDemuxBufferSize(demux_buffer_size);
#elif defined(TS_WINDOWS)
    tuner.setSinkQueueSize(demux_queue_size);
#endif

    return true;
}


//----------------------------------------------------------------------------
// If a channel name was specified instead of modulation parameters,
// load its description from the channel file.
//----------------------------------------------------------------------------

bool ts::TunerArgs::resolveChannel(const DeliverySystemSet& systems, Report& report)
{
    if (channel_name.empty()) {
        // No channel name to resolve.
        return true;
    }
    else {
        ChannelFile file;
        return file.load(tuning_file_name, report) && file.serviceToTuning(*this, systems, channel_name, false, report);
    }
}
