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

#include "tsSystemMonitor.h"
#include "tsGuardCondition.h"
#include "tsxmlModelDocument.h"
#include "tsxmlElement.h"
#include "tsIntegerUtils.h"
#include "tsForkPipe.h"
#include "tsSysUtils.h"
#include "tsTime.h"

// Stack size for the monitor thread
#define MONITOR_STACK_SIZE (64 * 1024)


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SystemMonitor::SystemMonitor(Report& report, const UString& config) :
    Thread(ThreadAttributes().setPriority(ThreadAttributes::GetMinimumPriority()).setStackSize(MONITOR_STACK_SIZE)),
    _report(report),
    _config_file(config),
    _periods(),
    _mutex(),
    _wake_up(),
    _terminate(false)
{
}

ts::SystemMonitor::~SystemMonitor()
{
    stop();
    waitForTermination();
}

ts::SystemMonitor::Config::Config() :
    log_messages(false),
    stable_memory(false),
    max_cpu(0),
    alarm_command()
{
}

ts::SystemMonitor::Period::Period() :
    Config(),
    duration(0),
    interval(0)
{
}


//----------------------------------------------------------------------------
// Prefix strings for all monitor messages (for filtering purpose)
//----------------------------------------------------------------------------

ts::UString ts::SystemMonitor::MonPrefix(const ts::Time& date)
{
    return u"[MON] " + date.format(ts::Time::DATE | ts::Time::HOUR | ts::Time::MINUTE) + u", ";
}


//----------------------------------------------------------------------------
// Stop the monitor thread.
//----------------------------------------------------------------------------

void ts::SystemMonitor::stop()
{
    GuardCondition lock(_mutex, _wake_up);
    _terminate = true;
    lock.signal();
}


//----------------------------------------------------------------------------
// Thread main code. Inherited from Thread
//----------------------------------------------------------------------------

void ts::SystemMonitor::main()
{
    // Load configuration file, consider as terminated on error.
    if (!loadConfigurationFile(_config_file)) {
        _report.error(u"monitoring ignored, invalid system monitoring XML file %s", {_config_file});
        return;
    }

    // Start with the first period. There must be at least one if the configuration is valid.
    auto period = _periods.begin();
    assert(period != _periods.end());
    size_t period_index = 0;

    // Last period.
    auto last_period = _periods.end();
    assert(last_period != _periods.begin());
    --last_period;

    // Starting time of next period.
    const Time start_time(Time::CurrentLocalTime());
    Time start_next_period(start_time + period->duration);

    // Get initial system metrics.
    ProcessMetrics start_metrics;
    GetProcessMetrics(start_metrics);

    // Time and metrics at the last interval.
    Time last_time(start_time);
    ProcessMetrics last_metrics(start_metrics);

    // Time and value of last virtual memory size increase.
    Time vsize_uptime(start_time);
    size_t vsize_max(start_metrics.vmem_size);

    _report.info(u"%sresource monitoring started", {MonPrefix(start_time)});
    bool mute_reported = false;

    // Loop on monitoring intervals.
    for (;;) {

        // Compute next time profile.
        const Time now(Time::CurrentLocalTime());
        while (period != last_period && now >= start_next_period) {
            period++;
            period_index++;
            start_next_period += period->duration;
            mute_reported = false;
            _report.debug(u"starting monitoring period #%d, duration: %'d ms, interval: %'d ms", {period_index, period->duration, period->interval});
        }

        // Wait until due time or termination request
        {
            GuardCondition lock(_mutex, _wake_up);
            if (!_terminate) {
                lock.waitCondition(period->interval);
            }
            if (_terminate) {
                break;
            }
        }

        // If we no longer log monitoring messages, issue a last message.
        if (!period->log_messages && !mute_reported) {
            _report.info(u"%sstopping stable monitoring messages to avoid infinitely large log files", {MonPrefix(Time::CurrentLocalTime())});
            mute_reported = true;
        }

        // Get current process metrics
        Time current_time(Time::CurrentLocalTime());
        ProcessMetrics metrics;
        GetProcessMetrics(metrics);

        // Build the monitoring message.
        UString message(MonPrefix(current_time));

        // Format virtual memory size status.
        message.format(u"VM: %s", {UString::HumanSize(metrics.vmem_size)});
        if (metrics.vmem_size != last_metrics.vmem_size) {
            // Virtual memory has changed
            message.format(u" (%s)", {UString::HumanSize(ptrdiff_t(metrics.vmem_size) - ptrdiff_t(last_metrics.vmem_size), u"B", true)});
        }
        else {
            // VM stable since last time. Check if temporarily stable or safely stable.
            // If no increase during last 95% of the running time, then we are really stable.
            message += (current_time - vsize_uptime) > (95 * (current_time - start_time)) / 100 ? u" (stable)" : u" (stabilizing)";
        }

        // Format CPU load.
        message += u", CPU:";
        message += UString::Percentage(metrics.cpu_time - last_metrics.cpu_time, current_time - last_time);
        message += u" (average:";
        message += UString::Percentage(metrics.cpu_time - start_metrics.cpu_time, current_time - start_time);
        message += u")";

        // Display monitoring message if allowed in this period or if vmem has increased.
        if (period->log_messages || metrics.vmem_size > vsize_max) {
            _report.info(message);
        }

        // Compute CPU percentage during last period.
        const int cpu = current_time <= last_time ? 0 : int((100 * (metrics.cpu_time - last_metrics.cpu_time)) / (current_time - last_time));

        // Raise an alarm if the CPU usage is above defined limit for this period.
        if (cpu > period->max_cpu) {
            _report.warning(u"%sALARM, CPU usage is %d%%, max defined to %d%%", {MonPrefix(current_time), cpu, period->max_cpu});
            if (!period->alarm_command.empty()) {
                UString command;
                command.format(u"%s \"%s\" cpu %d", {period->alarm_command, message, cpu});
                ForkPipe::Launch(command, _report, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE);
            }
        }

        // Raise an alarm if the virtual memory is not stable while it should be.
        if (period->stable_memory && metrics.vmem_size > last_metrics.vmem_size) {
            _report.warning(u"%sALARM, VM is not stable: %s in last monitoring interval",
                            {MonPrefix(current_time), UString::HumanSize(ptrdiff_t(metrics.vmem_size) - ptrdiff_t(last_metrics.vmem_size), u"B", true)});
            if (!period->alarm_command.empty()) {
                UString command;
                command.format(u"%s \"%s\" memory %d", {period->alarm_command, message, metrics.vmem_size});
                ForkPipe::Launch(command, _report, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE);
            }
        }

        // Remember points when virtual memory increases.
        if (metrics.vmem_size > vsize_max) {
            vsize_max = metrics.vmem_size;
            vsize_uptime = current_time;
        }

        // Save current metrics for next interval.
        last_time = current_time;
        last_metrics = metrics;
    }

    _report.info(u"%sresource monitoring terminated", {MonPrefix(Time::CurrentLocalTime())});
}


//----------------------------------------------------------------------------
// Laad the monitoring configuration file.
//----------------------------------------------------------------------------

bool ts::SystemMonitor::loadConfigurationFile(const UString& config)
{
    // Load the repository XML file. Search it in TSDuck directory if the default file is used.
    const bool use_default_config = config.empty();
    xml::Document doc(_report);
    if (!doc.load(use_default_config ? u"tsduck.monitor.xml" : config, use_default_config)) {
        return false;
    }

    // Load the XML model. Search it in TSDuck directory.
    xml::ModelDocument model(_report);
    if (!model.load(u"tsduck.monitor.model.xml", true)) {
        _report.error(u"Model for TSDuck system monitoring XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!model.validate(doc)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();

    // Get one required <defaults> entry, one required <profile> and one or more <period> entries.
    xml::ElementVector defaults;
    xml::ElementVector profiles;
    xml::ElementVector periods;
    Config defconfig;
    bool ok = root->getChildren(defaults, u"defaults", 1, 1) &&
              loadConfig(defconfig, defaults[0], nullptr) &&
              root->getChildren(profiles, u"profile", 1, 1) &&
              profiles[0]->getChildren(periods, u"period", 1);

    // Parse all <period> entries.
    for (auto it = periods.begin(); ok && it != periods.end(); ++it) {
        Period period;
        ok = (*it)->getIntAttribute(period.duration, u"duration", false, std::numeric_limits<MilliSecond>::max(), 1) &&
             (*it)->getIntAttribute(period.interval, u"interval", true, 0, 1) &&
             loadConfig(period, *it, &defconfig);
        // XML values are in seconds, we use milliseconds internally.
        period.duration *= MilliSecPerSec;
        period.interval *= MilliSecPerSec;
        _periods.push_back(period);
    }

    _report.debug(u"monitoring configuration loaded, %d periods", {_periods.size()});
    return ok;
}


//----------------------------------------------------------------------------
// Laad one configuration entry.
//----------------------------------------------------------------------------

bool ts::SystemMonitor::loadConfig(Config& config, const xml::Element* elem, const Config* defconfig)
{
    // Without default config, all fields are required.
    const bool required = defconfig == nullptr;
    bool ok = elem->getIntAttribute(config.max_cpu, u"max_cpu", required, required ? 0 : defconfig->max_cpu, 0, 100) &&
              elem->getBoolAttribute(config.stable_memory, u"stable_memory", required, required ? false : defconfig->stable_memory) &&
              elem->getBoolAttribute(config.log_messages, u"log", required, required ? false : defconfig->log_messages) &&
              elem->getTextChild(config.alarm_command, u"alarm", true, false, required ? UString() : defconfig->alarm_command);

    // Remove all newlines in the alarm command.
    config.alarm_command.remove(LINE_FEED);
    config.alarm_command.remove(CARRIAGE_RETURN);
    return ok;
}
