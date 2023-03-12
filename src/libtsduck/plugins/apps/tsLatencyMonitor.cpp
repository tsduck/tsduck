//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Amos Cheung
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

#include "tsLatencyMonitor.h"
#include "tstslatencymonitorInputExecutor.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::LatencyMonitor::LatencyMonitor(const LatencyMonitorArgs& args, Report& report) :
    _report(report),
    _args(args),
    _inputs(),
    _mutex(),
    _max_latency(0),
    _last_output_time(Time::Epoch),
    _output_stream(),
    _output_file(nullptr)
{
    // Debug message.
    if (_report.debug()) {
        UString cmd(args.appName);
        cmd.append(u" ");
        for (const auto& input : args.inputs) {
            cmd.append(u" ");
            cmd.append(input.toString(PluginType::INPUT));
        }
        _report.debug(u"starting: %s", {cmd});
    }

    // Clear errors on the report, used to check further initialisation errors.
    _report.resetErrors();

    // Get all input plugin options.
    for (size_t i = 0; i < _args.inputs.size(); ++i) {
        auto inputExecutor = std::make_shared<tslatencymonitor::InputExecutor>(_args, i, *this, _report);
        _inputs.push_back(InputData{inputExecutor, {}});
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
        if (!_inputs[i].inputExecutor -> plugin()->getOptions()) {
            return false;
        }
    }

    // Create the output file if there is one
    if (_args.outputName.empty()) {
        _output_file = &std::cerr;
    }
    else {
        _output_file = &_output_stream;
        _output_stream.open(_args.outputName.toUTF8().c_str());
        if (!_output_stream) {
            return false;
        }
    }

    // Output header
    csvHeader();

    // Start all input threads
    for (size_t i = 0; i < _inputs.size(); ++i) {
        // Here, start() means start the thread, and start input plugin.
        bool success = _inputs[i].inputExecutor->start();
        if (!success) {
            return false;
        }
    }

    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i].inputExecutor->waitForTermination();
    }

    return true;
}


//----------------------------------------------------------------------------
// Pass incoming TS packets for processing (called by input plugins).
//----------------------------------------------------------------------------

void ts::LatencyMonitor::processPacket(const TSPacketVector& pkt, const TSPacketMetadataVector& metadata, size_t count, size_t pluginIndex)
{
    GuardMutex lock(_mutex);
    TimingDataList& timingDataList = _inputs[pluginIndex].timingDataList;

    for (size_t i = 0; i < count; i++) {
        uint64_t pcr = pkt[i].getPCR();
        const bool has_pcr = pcr != INVALID_PCR;
        if (has_pcr) {
            uint64_t timestamp = metadata[i].getInputTimeStamp();
            // Checking to see if the buffer time has been reached, and pop back element (oldest element) if the buffer time has been reached
            while (!timingDataList.empty() && (timestamp - timingDataList.back().timestamp) / SYSTEM_CLOCK_FREQ  >= _args.bufferTime) {
                timingDataList.pop_back();
            }
            timingDataList.push_front(TimingData{pcr, timestamp});
        }
    }

    // Check whether the elapsed time since the last output exceeds the output interval (in seconds)
    uint64_t timeDiff = (Time::CurrentUTC() - _last_output_time) / 1000;
    if (timeDiff >= _args.outputInterval) {
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
    *_output_file << "PCR1" << TS_DEFAULT_CSV_SEPARATOR
                  << "PCR2" << TS_DEFAULT_CSV_SEPARATOR
                  << "Latency (ms)" << TS_DEFAULT_CSV_SEPARATOR
                  << "Max Latency (ms)"
                  << std::endl;
}


//----------------------------------------------------------------------------
// Calculate delta of two PCRs
//----------------------------------------------------------------------------

void ts::LatencyMonitor::calculatePCRDelta(InputDataVector& inputs)
{
    // Check if both timing data lists not empty
    if (inputs[0].timingDataList.empty() && inputs[1].timingDataList.empty()) {
        return;
    }

    TimingDataList& timingDataList1 = inputs[0].timingDataList;
    TimingDataList& timingDataList2 = inputs[1].timingDataList;

    TimingData timingData1 = timingDataList1.front();
    TimingData timingData2 = timingDataList2.front();

    bool retry = false;
    do {
        TimingDataList* refTimingDataList = nullptr;
        TimingDataList* shiftTimingDataList = nullptr;

        if (!retry) {
            // If not retrying, use the list with the smaller PCR as the reference list and the list with the larger PCR as the shifted list
            refTimingDataList = (timingData1.pcr > timingData2.pcr) ? &timingDataList2 : &timingDataList1;
            shiftTimingDataList = (timingData1.pcr > timingData2.pcr) ? &timingDataList1 : &timingDataList2;
        }
        else {
            // If retrying, use the list with the larger PCR as the reference list and the list with the smaller PCR as the shifted list (loop point handling)
            refTimingDataList = (timingData1.pcr > timingData2.pcr) ? &timingDataList1 : &timingDataList2;
            shiftTimingDataList = (timingData1.pcr > timingData2.pcr) ? &timingDataList2 : &timingDataList1;
        }

        // Find the matching PCR in the shifted list
        for (auto data = shiftTimingDataList->begin(); data != shiftTimingDataList->end(); ++data) {
            TimingData refTimingData = refTimingDataList->front();
            TimingData shiftTimingData = *data;

            if (refTimingData.pcr == shiftTimingData.pcr) {
                // Calculate the PCR delta if the PCR matches
                int64_t pcrDelta = std::abs(int64_t(refTimingData.timestamp) - int64_t(shiftTimingData.timestamp));
                double latency = double(pcrDelta) / SYSTEM_CLOCK_FREQ * 1000;
                _max_latency = std::max(_max_latency, latency);

                *_output_file << (refTimingDataList == &timingDataList1 ? refTimingData.pcr : shiftTimingData.pcr) << TS_DEFAULT_CSV_SEPARATOR
                            << (refTimingDataList == &timingDataList2 ? refTimingData.pcr : shiftTimingData.pcr) << TS_DEFAULT_CSV_SEPARATOR
                            << latency << TS_DEFAULT_CSV_SEPARATOR
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
    *_output_file << ((timingDataList1.empty()) ? "LOST" : std::to_string(timingDataList1.front().pcr)) << TS_DEFAULT_CSV_SEPARATOR
                  << ((timingDataList2.empty()) ? "LOST" : std::to_string(timingDataList2.front().pcr)) << TS_DEFAULT_CSV_SEPARATOR
                  << "N/A" << TS_DEFAULT_CSV_SEPARATOR
                  << "N/A" << std::endl;
}
