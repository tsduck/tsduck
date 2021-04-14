//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsMuxerArgs.h"
#include "tsArgsWithPlugins.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::MuxerArgs::DEFAULT_MAX_INPUT_PACKETS;
constexpr size_t ts::MuxerArgs::MIN_INPUT_PACKETS;
constexpr size_t ts::MuxerArgs::DEFAULT_MAX_OUTPUT_PACKETS;
constexpr size_t ts::MuxerArgs::MIN_OUTPUT_PACKETS;
constexpr size_t ts::MuxerArgs::DEFAULT_BUFFERED_PACKETS;
constexpr size_t ts::MuxerArgs::MIN_BUFFERED_PACKETS;
constexpr ts::MilliSecond ts::MuxerArgs::DEFAULT_RESTART_DELAY;
#endif


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::MuxerArgs::MuxerArgs() :
    appName(),
    inputs(),
    output(),
    outputBitRate(0),
    lossyInput(false),
    inputOnce(false),
    outputOnce(false),
    inputRestartDelay(DEFAULT_RESTART_DELAY),
    outputRestartDelay(DEFAULT_RESTART_DELAY),
    inBufferPackets(DEFAULT_BUFFERED_PACKETS),
    outBufferPackets(DEFAULT_BUFFERED_PACKETS),
    maxInputPackets(DEFAULT_MAX_INPUT_PACKETS),
    maxOutputPackets(DEFAULT_MAX_OUTPUT_PACKETS)
{
}


//----------------------------------------------------------------------------
// Enforce default or minimum values.
//----------------------------------------------------------------------------

void ts::MuxerArgs::enforceDefaults()
{
    if (inputs.empty()) {
        // If no input plugin is used, used only standard input.
        inputs.push_back(PluginOptions(u"file"));
    }
    if (output.name.empty()) {
        output.set(u"file");
    }
    inBufferPackets = std::max(inBufferPackets, MIN_BUFFERED_PACKETS);
    outBufferPackets = std::max(outBufferPackets, inputs.size() * inBufferPackets);
    maxInputPackets = std::min(std::max(maxInputPackets, MIN_INPUT_PACKETS), inBufferPackets / 2);
    maxOutputPackets = std::max(maxOutputPackets, MIN_OUTPUT_PACKETS);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::MuxerArgs::defineArgs(Args& args) const
{
    args.option<BitRate>(u"bitrate", 'b');
    args.help(u"bitrate",
              u"Specify the target constant output bitrate in bits per seconds. "
              u"In most cases, this is a required parameter. "
              u"Without explicit bitrate, the output plugin must be able to report "
              u"its bitrate immediately after starting. "
              u"This is typically possible on modulators and ASI cards only.");

    args.option(u"buffer-packets", 0, Args::POSITIVE);
    args.help(u"buffer-packets",
              u"Specify the size in TS packets of each input plugin buffer. "
              u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" packets. "
              u"The size of the output buffer is the sum of all input buffers sizes.");

    args.option(u"lossy-input");
    args.help(u"lossy-input",
              u"When an input plugin provides packets faster than the output consumes them, "
              u"drop older buffered input packets in order to read more recent packets. "
              u"By default, block an input plugin when its buffer is full.");

    args.option(u"max-input-packets", 0, Args::POSITIVE);
    args.help(u"max-input-packets",
              u"Specify the maximum number of TS packets to read at a time. "
              u"This value may impact the switch response time. "
              u"The default is " + UString::Decimal(DEFAULT_MAX_INPUT_PACKETS) + u" packets. "
              u"The actual value is never more than half the --buffer-packets value.");

    args.option(u"max-output-packets", 0, Args::POSITIVE);
    args.help(u"max-output-packets",
              u"Specify the maximum number of TS packets to write at a time. "
              u"The default is " + UString::Decimal(DEFAULT_MAX_OUTPUT_PACKETS) + u" packets.");

    args.option(u"restart-delay", 0, Args::UNSIGNED);
    args.help(u"restart-delay",
              u"Specify a restart delay in milliseconds for plugins. "
              u"When a plugin fails or terminates, it is immediately restarted. "
              u"In case of initial restart error, wait the specified delay before retrying. "
              u"The default is " + UString::Decimal(DEFAULT_RESTART_DELAY) + u" ms.");

    args.option(u"terminate", 't');
    args.help(u"terminate",
              u"Terminate execution when all input plugins complete, do not restart plugins. "
              u"By default, restart input plugins when they terminate or fail.");

    args.option(u"terminate-with-output");
    args.help(u"terminate-with-output",
              u"Terminate execution when the output plugin fails, do not restart. "
              u"By default, restart the output plugin when it fails.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::MuxerArgs::loadArgs(DuckContext& duck, Args& args)
{
    appName = args.appName();
    lossyInput = args.present(u"lossy-input");
    inputOnce = args.present(u"terminate");
    outputOnce = args.present(u"terminate-with-output");
    args.getFixedValue(outputBitRate, u"bitrate");
    args.getIntValue(inputRestartDelay, u"restart-delay", DEFAULT_RESTART_DELAY);
    outputRestartDelay = inputRestartDelay;
    args.getIntValue(inBufferPackets, u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    args.getIntValue(maxInputPackets, u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS);
    args.getIntValue(maxOutputPackets, u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);

    // Load all plugin descriptions. Default output is the standard output file.
    ArgsWithPlugins* pargs = dynamic_cast<ArgsWithPlugins*>(&args);
    if (pargs != nullptr) {
        pargs->getPlugins(inputs, PluginType::INPUT);
        pargs->getPlugin(output, PluginType::OUTPUT, u"file");
    }
    else {
        inputs.clear();
        output.set(u"file");
    }
    if (inputs.empty()) {
        // If no input plugin is used, used only standard input.
        inputs.push_back(PluginOptions(u"file"));
    }

    // Default output buffer size is the sum of all input buffer sizes.
    outBufferPackets = inputs.size() * inBufferPackets;

    // Enforce defaults and other invalid values.
    enforceDefaults();

    return args.valid();
}
