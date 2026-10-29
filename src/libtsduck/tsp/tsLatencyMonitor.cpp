//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLatencyMonitor.h"
#include "tstslatencymonitorInputExecutor.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::LatencyMonitor::LatencyMonitor(const LatencyMonitorArgs& args, Report& report) :
    _report(report),
    _args(args)
{
    // Debug message.
    if (_report.debug()) {
        UString cmd(args.app_name);
        cmd.append(u" ");
        for (const auto& input : args.inputs) {
            cmd.append(u" ");
            cmd.append(input.toString(PluginType::INPUT));
        }
        _report.debug(u"starting: %s", cmd);
    }

    // Clear errors on the report, used to check further initialisation errors.
    _report.resetErrors();

    // Get all input plugin options.
    for (size_t i = 0; i < _args.inputs.size(); ++i) {
        _inputs.push_back(InputData{std::make_shared<tslatencymonitor::InputExecutor>(_args, i, *this, _report), {}});
    }

    // Init last output time
    _last_output_time = Time::CurrentUTC();
}


//----------------------------------------------------------------------------
// Start the PCR comparator session.
//----------------------------------------------------------------------------

bool ts::LatencyMonitor::start()
{
    // Get all input plugin options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        if (!_inputs[i].input_executor -> plugin()->getOptions()) {
            return false;
        }
    }

    // Create the output file if there is one
    if (_args.output_name.empty()) {
        _output_file = &std::cerr;
    }
    else {
        _output_file = &_output_stream;
        _output_stream.open(_args.output_name);
        if (!_output_stream) {
            return false;
        }
    }

    // Output header
    csvHeader();

    // Start all input threads
    for (size_t i = 0; i < _inputs.size(); ++i) {
        // Here, start() means start the thread, and start input plugin.
        bool success = _inputs[i].input_executor->start();
        if (!success) {
            return false;
        }
    }

    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i].input_executor->waitForTermination();
    }

    return true;
}


//----------------------------------------------------------------------------
// Pass incoming TS packets for processing (called by input plugins).
//----------------------------------------------------------------------------

void ts::LatencyMonitor::processPacket(const TSPacketVector& pkt, const TSPacketMetadataVector& metadata, size_t count, size_t pluginIndex)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    TimingDataList& timing_data_list(_inputs[pluginIndex].timing_data_list);

    for (size_t i = 0; i < count; i++) {
        uint64_t pcr = pkt[i].getPCR();
        const bool has_pcr = pcr != INVALID_PCR;
        if (has_pcr) {
            const PCR timestamp = metadata[i].getInputTimeStamp();
            // Checking to see if the buffer time has been reached, and pop back element (oldest element) if the buffer time has been reached
            while (!timing_data_list.empty() && (timestamp - timing_data_list.back().timestamp) >= _args.buffer_time) {
                timing_data_list.pop_back();
            }
            timing_data_list.push_front(TimingData{pcr, timestamp});
        }
    }

    // Check whether the elapsed time since the last output exceeds the output interval.
    const cn::milliseconds time_diff = cn::milliseconds(Time::CurrentUTC() - _last_output_time);
    if (time_diff >= _args.output_interval) {
        // Set output timer to current time
        _last_output_time = Time::CurrentUTC();
        calculatePCRDelta(_inputs);
    }
}


//----------------------------------------------------------------------------
// Generate csv header
//----------------------------------------------------------------------------

void ts::LatencyMonitor::csvHeader()
{
    *_output_file << "PCR1" << DEFAULT_CSV_SEPARATOR
                  << "PCR2" << DEFAULT_CSV_SEPARATOR
                  << "Latency (ms)" << DEFAULT_CSV_SEPARATOR
                  << "Max Latency (ms)"
                  << std::endl;
}


//----------------------------------------------------------------------------
// Calculate delta of two PCRs
//----------------------------------------------------------------------------

void ts::LatencyMonitor::calculatePCRDelta(InputDataVector& inputs)
{
    // Check if both timing data lists not empty
    if (inputs.size() != 2 || inputs[0].timing_data_list.empty() || inputs[1].timing_data_list.empty()) {
        return;
    }

    TimingDataList& timing_data_list1 = inputs[0].timing_data_list;
    TimingDataList& timing_data_list2 = inputs[1].timing_data_list;

    TimingData timing_data1 = timing_data_list1.front();
    TimingData timing_data2 = timing_data_list2.front();

    bool retry = false;
    do {
        TimingDataList* ref_timing_data_list = nullptr;
        TimingDataList* shift_timing_data_list = nullptr;

        if (!retry) {
            // If not retrying, use the list with the smaller PCR as the reference list and the list with the larger PCR as the shifted list
            ref_timing_data_list = (timing_data1.pcr > timing_data2.pcr) ? &timing_data_list2 : &timing_data_list1;
            shift_timing_data_list = (timing_data1.pcr > timing_data2.pcr) ? &timing_data_list1 : &timing_data_list2;
        }
        else {
            // If retrying, use the list with the larger PCR as the reference list and the list with the smaller PCR as the shifted list (loop point handling)
            ref_timing_data_list = (timing_data1.pcr > timing_data2.pcr) ? &timing_data_list1 : &timing_data_list2;
            shift_timing_data_list = (timing_data1.pcr > timing_data2.pcr) ? &timing_data_list2 : &timing_data_list1;
        }

        // Find the matching PCR in the shifted list
        for (auto data = shift_timing_data_list->begin(); data != shift_timing_data_list->end(); ++data) {
            TimingData ref_timing_data = ref_timing_data_list->front();
            TimingData shift_iming_data = *data;

            if (ref_timing_data.pcr == shift_iming_data.pcr) {
                // Calculate the PCR delta if the PCR matches
                const PCR::rep pcrDelta = std::abs(ref_timing_data.timestamp.count() - shift_iming_data.timestamp.count());
                const double latency = double(pcrDelta) / SYSTEM_CLOCK_FREQ * 1000;
                _max_latency = std::max(_max_latency, latency);

                *_output_file << (ref_timing_data_list == &timing_data_list1 ? ref_timing_data.pcr : shift_iming_data.pcr) << DEFAULT_CSV_SEPARATOR
                              << (ref_timing_data_list == &timing_data_list2 ? ref_timing_data.pcr : shift_iming_data.pcr) << DEFAULT_CSV_SEPARATOR
                              << latency << DEFAULT_CSV_SEPARATOR
                              << _max_latency << std::endl;

                return;
            }
        }

        // Determine whether to retry after no match PCR pair is found
        if (!retry) {
            // If it is not retrying, set retry to true and retry
            retry = true;
        }
        else {
            // If it is retrying, exit from the loop
            break;
        }
    } while (retry);

    // Output the latest PCR from both list with "LOST" if PCR doesn't exist
    *_output_file << ((timing_data_list1.empty()) ? "LOST" : std::to_string(timing_data_list1.front().pcr)) << DEFAULT_CSV_SEPARATOR
                  << ((timing_data_list2.empty()) ? "LOST" : std::to_string(timing_data_list2.front().pcr)) << DEFAULT_CSV_SEPARATOR
                  << "N/A" << DEFAULT_CSV_SEPARATOR
                  << "N/A" << std::endl;
}
