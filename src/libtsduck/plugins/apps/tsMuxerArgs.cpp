//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMuxerArgs.h"
#include "tsArgsWithPlugins.h"


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
    lossyReclaim = std::min(std::max<size_t>(1, lossyReclaim), inBufferPackets);
    patBitRate = patBitRate.max(MIN_PSI_BITRATE);
    catBitRate = catBitRate.max(MIN_PSI_BITRATE);
    nitBitRate = nitBitRate.max(MIN_PSI_BITRATE);
    sdtBitRate = sdtBitRate.max(MIN_PSI_BITRATE);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::MuxerArgs::defineArgs(Args& args)
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

    args.option<cn::microseconds>(u"cadence");
    args.help(u"cadence",
              u"Specify the internal polling cadence in microseconds. "
              u"The default is " + UString::Chrono(DEFAULT_CADENCE) + u".");

    args.option<BitRate>(u"cat-bitrate", 0, 0, 0, MIN_PSI_BITRATE);
    args.help(u"cat-bitrate",
              u"CAT bitrate in output stream. The default is " + UString::Decimal(DEFAULT_PSI_BITRATE) + u" b/s.");

    args.option(u"eit", 0, TableScopeEnum());
    args.help(u"eit", u"type",
              u"Specify which type of EIT shall be merged in the output stream. The default is \"actual\".");

    args.option(u"ignore-conflicts", 'i');
    args.help(u"ignore-conflicts",
              u"Ignore PID or service conflicts. The resultant output stream will be inconsistent. "
              u"By default, a PID or service conflict between input stream aborts the processing.");

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

    args.option(u"nit", 0, TableScopeEnum());
    args.help(u"nit", u"type",
              u"Specify which type of NIT shall be merged in the output stream. The default is \"actual\".");

    args.option<BitRate>(u"nit-bitrate", 0, 0, 0, MIN_PSI_BITRATE);
    args.help(u"nit-bitrate",
              u"NIT bitrate in output stream. The default is " + UString::Decimal(DEFAULT_PSI_BITRATE) + u" b/s.");

    args.option(u"original-network-id", 0, Args::UINT16);
    args.help(u"original-network-id",
              u"Specify the original network id of the output stream. The default is 0.");

    args.option<BitRate>(u"pat-bitrate", 0, 0, 0, MIN_PSI_BITRATE);
    args.help(u"pat-bitrate",
              u"PAT bitrate in output stream. The default is " + UString::Decimal(DEFAULT_PSI_BITRATE) + u" b/s.");

    args.option<cn::milliseconds>(u"restart-delay");
    args.help(u"restart-delay",
              u"Specify a restart delay for plugins. "
              u"When a plugin fails or terminates, it is immediately restarted. "
              u"In case of initial restart error, wait the specified delay before retrying. "
              u"The default is " + UString::Chrono(DEFAULT_RESTART_DELAY, true) + u".");

    args.option(u"sdt", 0, TableScopeEnum());
    args.help(u"sdt", u"type",
              u"Specify which type of SDT shall be merged in the output stream. The default is \"actual\".");

    args.option<BitRate>(u"sdt-bitrate", 0, 0, 0, MIN_PSI_BITRATE);
    args.help(u"sdt-bitrate",
              u"SDT bitrate in output stream. The default is " + UString::Decimal(DEFAULT_PSI_BITRATE) + u" b/s.");

    args.option(u"terminate", 't');
    args.help(u"terminate",
              u"Terminate execution when all input plugins complete, do not restart plugins. "
              u"By default, restart input plugins when they terminate or fail.");

    args.option(u"terminate-with-output");
    args.help(u"terminate-with-output",
              u"Terminate execution when the output plugin fails, do not restart. "
              u"By default, restart the output plugin when it fails.");

    args.option(u"time-reference-input", 0, Args::UNSIGNED);
    args.help(u"time-reference-input",
              u"Specify the index of the input plugin from which the time reference PID (TDT/TOT) is copied into the output stream. "
              u"The time reference PID of all other input streams is discarded. "
              u"By default, the first input stream which produces a time reference table will be used.");

    args.option(u"ts-id", 0, Args::UINT16);
    args.help(u"ts-id",
              u"Specify the transport stream id of the output stream. The default is 0.");
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
    ignoreConflicts = args.present(u"ignore-conflicts");
    args.getValue(outputBitRate, u"bitrate");
    args.getChronoValue(inputRestartDelay, u"restart-delay", DEFAULT_RESTART_DELAY);
    outputRestartDelay = inputRestartDelay;
    args.getChronoValue(cadence, u"cadence", DEFAULT_CADENCE);
    args.getIntValue(inBufferPackets, u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    args.getIntValue(maxInputPackets, u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS);
    args.getIntValue(maxOutputPackets, u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);
    args.getIntValue(outputTSId, u"ts-id", 0);
    args.getIntValue(outputNetwId, u"original-network-id", 0);
    args.getIntValue(nitScope, u"nit", TableScope::ACTUAL);
    args.getIntValue(sdtScope, u"sdt", TableScope::ACTUAL);
    args.getIntValue(eitScope, u"eit", TableScope::ACTUAL);
    args.getIntValue(timeInputIndex, u"time-reference-input", NPOS);
    args.getValue(patBitRate, u"pat-bitrate", DEFAULT_PSI_BITRATE);
    args.getValue(catBitRate, u"cat-bitrate", DEFAULT_PSI_BITRATE);
    args.getValue(nitBitRate, u"nit-bitrate", DEFAULT_PSI_BITRATE);
    args.getValue(sdtBitRate, u"sdt-bitrate", DEFAULT_PSI_BITRATE);

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
    if (timeInputIndex != NPOS && timeInputIndex >= inputs.size()) {
        args.error(u"%d is not a valid input plugin index in --time-reference-input", timeInputIndex);
    }

    // Default output buffer size is the sum of all input buffer sizes.
    outBufferPackets = inputs.size() * inBufferPackets;

    // Get default options for TSDuck contexts in each plugin.
    duck.saveArgs(duckArgs);

    // Enforce defaults and other invalid values.
    enforceDefaults();

    return args.valid();
}
