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

#include "tsTunerArgs.h"
#include "tsTuner.h"
#include "tsChannelFile.h"
#include "tsDuckContext.h"
#include "tsHFBand.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TunerArgs::TunerArgs(bool info_only) :
    ModulationArgs(),
    device_name(),
    signal_timeout(Tuner::DEFAULT_SIGNAL_TIMEOUT),
    receive_timeout(0),
    demux_buffer_size(Tuner::DEFAULT_DEMUX_BUFFER_SIZE),
    demux_queue_size(Tuner::DEFAULT_SINK_QUEUE_SIZE),
    receiver_name(),
    _info_only(info_only)
{
}


//----------------------------------------------------------------------------
// Reset all values, they become "unset"
//----------------------------------------------------------------------------

void ts::TunerArgs::clear()
{
    device_name.clear();
    signal_timeout = Tuner::DEFAULT_SIGNAL_TIMEOUT;
    receive_timeout = 0;
    demux_buffer_size = Tuner::DEFAULT_DEMUX_BUFFER_SIZE;
    demux_queue_size = Tuner::DEFAULT_SINK_QUEUE_SIZE;
    receiver_name.clear();

    // Reset superclass.
    ModulationArgs::clear();
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TunerArgs::loadArgs(DuckContext& duck, Args& args)
{
    bool status = true;
    TunerArgs::clear();

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

#if defined(TS_WINDOWS)
    // Windows-specific tuner options.
    receiver_name = args.value(u"receiver-name");
#endif

    // Tuning options.
    if (!_info_only) {
        // Reception parameters.
        signal_timeout = args.intValue<MilliSecond>(u"signal-timeout", Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) * 1000;
        receive_timeout = args.intValue<MilliSecond>(u"receive-timeout", receive_timeout); // preserve previous value
#if defined(TS_LINUX)
        demux_buffer_size = args.intValue<size_t>(u"demux-buffer-size", Tuner::DEFAULT_DEMUX_BUFFER_SIZE);
#elif defined(TS_WINDOWS)
        demux_queue_size = args.intValue<size_t>(u"demux-queue-size", Tuner::DEFAULT_SINK_QUEUE_SIZE);
#endif

        // Locating the transponder by channel
        const UString channel_name(args.value(u"channel-transponder"));
        if (!channel_name.empty()) {
            bool channel_found = false;

            // Check if the channel has the format "name-number". Split the name in two fields.
            UStringVector fields;
            channel_name.split(fields, u'-', true, true);

            // Try to match a channel number in an HF band.
            uint32_t channel_number = 0;
            const HFBand* band = nullptr;
            if (fields.size() == 2 && fields[1].toInteger(channel_number) && (band = duck.hfBand(fields[0], true)) != nullptr) {
                // Found a band name and channel number. Find associated frequency.
                // The option --offset-count is defined in superclass ModulationArgs.
                const uint64_t freq = band->frequency(channel_number, args.intValue<int32_t>(u"offset-count", 0));
                // If the returned frequency is zero, the channel is outside the band.
                if (freq != 0) {
                    channel_found = true;
                    frequency = freq;
                    // Some satellite bands define polarization in addition to frequency.
                    const Polarization pol = band->polarization(channel_number);
                    if (pol != POL_NONE && pol != POL_AUTO) {
                        polarity = pol;
                    }
                }
            }

            // If not found as an HF "band-number", try a "TV channel" name.
            if (!channel_found) {
                // To find a match, we need to know the delivery systems which are supported by the tuner.
                // And to do that, we need to temporarily open the tuner in "info only" mode.
                ChannelFile file;
                Tuner tuner(duck);
                _info_only = true;
                if (file.load(args.value(u"tuning-file"), duck.report()) && configureTuner(tuner)) {
                    channel_found = file.serviceToTuning(*this, tuner.deliverySystems(), channel_name, false, duck.report());
                    tuner.close();
                }
                _info_only = false;
            }

            status = status && channel_found;
        }

        // Other tuning parameters from superclass.
        status = ModulationArgs::loadArgs(duck, args) && status;
    }

    // Mark arguments as invalid is some errors were found.
    if (!status) {
        args.invalidate();
    }
    return status;
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TunerArgs::defineArgs(Args& args, bool allow_short_options)
{
    // Tuner identification.
    args.option(u"adapter", allow_short_options ? u'a' : 0, Args::UNSIGNED);
    args.help(u"adapter", u"N",
#if defined(TS_LINUX)
              u"Specifies the Linux DVB adapter N (/dev/dvb/adapterN). "
#elif defined(TS_WINDOWS)
              u"Specifies the Nth tuner device in the system. "
#endif
              u"This option can be used instead of device name.");

    args.option(u"device-name", allow_short_options ? u'd' : 0, Args::STRING);
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

#if defined(TS_WINDOWS)
    args.option(u"receiver-name", 0, Args::STRING);
    args.help(u"receiver-name", u"name",
        u"Specify the name of the DirectShow receiver filter to use. "
        u"By default, first try a direct connection from the tuner filter to the rest of the graph. "
        u"Then, try all receiver filters and concatenate them all.");
#endif

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
        ModulationArgs::defineArgs(args, allow_short_options);

        // Tuning using a channel configuration file.
        args.option(u"channel-transponder", allow_short_options ? 'c' : 0, Args::STRING);
        args.help(u"channel-transponder", u"name",
                  u"Tune to the transponder containing the specified channel. "
                  u"The channel name is not case-sensitive and blanks are ignored. "
                  u"It is either an \"HF band channel\" or a \"TV channel\".\n\n"
                  u"An \"HF band channel\" has the format \"band-number\" such as \"UHF-22\" "
                  u"(terrestrial) or \"BS-12\" (Japanese satellite). See also option --offset-count.\n\n"
                  u"A \"TV channel\" name is searched in a \"tuning file\" and the corresponding "
                  u"tuning information in this file is used. See also option --tuning-file.");

        args.option(u"tuning-file", 0, Args::FILENAME);
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

bool ts::TunerArgs::configureTuner(Tuner& tuner) const
{
    if (tuner.isOpen()) {
        tuner.report().error(u"tuner is already open");
        return false;
    }

    // These options shall be set before open().
    tuner.setReceiverFilterName(receiver_name);

    // Open DVB tuner. Use first device by default (if device name is empty).
    if (!tuner.open(device_name, _info_only)) {
        return false;
    }

    // Set configuration parameters.
    if (!_info_only) {
        tuner.setSignalTimeout(signal_timeout);
        if (!tuner.setReceiveTimeout(receive_timeout)) {
            tuner.report().error(u"failed to set tuner receive timeout");
            tuner.close(true);
            return false;
        }
        tuner.setSignalPoll(Tuner::DEFAULT_SIGNAL_POLL);
        tuner.setDemuxBufferSize(demux_buffer_size);
        tuner.setSinkQueueSize(demux_queue_size);
    }

    return true;
}
