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
//
//  Transport stream processor shared library:
//  Monitor EMM cycle periods
//  Copyright 2005-2011, Jerome Leveque
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsPIDOperator.h"
#include "tsFormat.h"
#include "tsCAT.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EMMMonitorPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        EMMMonitorPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:

        // Default values
        static const uint8_t DEFAULT_CYCLE_FAST = 0x03;
        static const uint8_t DEFAULT_CYCLE_MEDIUM = 0x02;
        static const uint8_t DEFAULT_CYCLE_SLOW = 0x01;

        static const uint16_t DEFAULT_FAST_MIN = 280;
        static const uint16_t DEFAULT_FAST_MAX = 600;
        static const uint16_t DEFAULT_MEDIUM_MIN = 1780;
        static const uint16_t DEFAULT_MEDIUM_MAX = 3600;
        static const uint16_t DEFAULT_SLOW_MIN = 3580;
        static const uint16_t DEFAULT_SLOW_MAX = 7200;

        static const uint32_t DEFAULT_UEMM_INTERVAL = 86460; // 24 hours and 1 minute


        // Parameters relative to an EMM cycle
        struct CycleParameters
        {
            time_t last_broadcast_date;   // last braodcast date of corresponding technical EMM
            uint16_t min_period;            // Minimum allowed period value
            uint16_t max_period;            // Maximum allowed period value

            // Constructor
            CycleParameters () : last_broadcast_date(0), min_period(0), max_period(0) {}
            CycleParameters (const time_t date, const uint16_t min, const uint16_t max) : 
            last_broadcast_date(date), min_period(min), max_period(max) {}
        };

        uint16_t                           _cas_id;         // CA system id for ECM or EMM
        CASFamily                        _cas_family;     // CA system id family
        PIDSet                           _emm_pids;       // List of EMM PIDs
        SectionDemux                     _demux;          // Section filter
        std::map<uint8_t, CycleParameters> _cycle_params;   // EMM cycle parameters 
        std::string                      _alarm_command;  // Alarm command name
        uint32_t                           _smartcard;      // Smartcard number to look for EMMs
                                                          // If zero, means no smartcard specified
        uint32_t                           _uemm_interval;  // Max interval between two different
                                                          // EMMs for the specified smartcard
        time_t                           _next_uemm_date; // Max date for the next unique EMM
        Section                          _uemm_section;   // Section with the last received EMM


        // Parse a string with cycle options, and update the cycles' list.
        bool parseCycleOptions (const std::string& options);

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Process specific tables
        void processCAT (const CAT&);
        void processTechnicalEMM (const BinaryTable& table);
        void processUniqueEMM (const BinaryTable& table);

        // Add all ECM/EMM PIDs from the specified list if they match
        // the optional selected CAS operator id.
        void addECMM (const PIDOperatorSet& pidop, const char *name);

        // Run the alarm command.
        void runAlarmCommand(const std::string& parameter);
    };
}

TSPLUGIN_DECLARE_VERSION;
TSPLUGIN_DECLARE_PROCESSOR (ts::EMMMonitorPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EMMMonitorPlugin::EMMMonitorPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "Monitor SafeAccess EMM broadcast.", "[options]"),
    _demux (this)
{
    option ("cycle",         'c', STRING, 0, UNLIMITED_COUNT);
    option ("alarm_command", 'a', STRING);
    option ("smartcard",     's', STRING);
    option ("emm_interval",    0, UINT32);

    setHelp ("Options:\n"
             "\n"
             "  -a command\n"
             "  --alarm_command command\n"
             "      Command to be run when an alarm is detected.\n"
             "\n"
             "  -c string\n"
             "  --cycle string\n"
             "      Give min and max bounds for this cycle's period.\n"
             "      Expected format is cycle_nb-min-max (eg 1-280-600).\n"
             "      Multiple occurences of this option are allowed.\n"
             "\n"
             "  -s value\n"
             "  --smartcard value\n"
             "      Smartcard number for which unique EMMs are monitored.\n"
             "      The complete reference (12 digits) is required.\n"
             "\n"
             "  --emm_interval value\n"
             "      Greatest allowed time interval (in seconds) between two\n"
             "      distinct (ie with a different content) unique EMMs addressed\n"
             "      to the specified smartcard.\n"
             "      Default value is 86460 s (1 day and 1 minute).\n"
             "      This parameter is ignored if no smartcard has been given\n"
             "      (with option -s or --smartcard).\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EMMMonitorPlugin::start()
{
    // Default cycle values
    _cycle_params[(uint8_t) DEFAULT_CYCLE_FAST] = CycleParameters(0, DEFAULT_FAST_MIN, DEFAULT_FAST_MAX);
    _cycle_params[(uint8_t) DEFAULT_CYCLE_MEDIUM] = CycleParameters(0, DEFAULT_MEDIUM_MIN, DEFAULT_MEDIUM_MAX);
    _cycle_params[(uint8_t) DEFAULT_CYCLE_SLOW] = CycleParameters(0, DEFAULT_SLOW_MIN, DEFAULT_SLOW_MAX);

    _cas_id = 0x4ADC;   // SafeAccess
    _cas_family = CASFamilyOf (_cas_id);


    // Get command line arguments

    // Alarm command
    _alarm_command = value("alarm_command");

    // Cycle(s) parameters
    // Several sets of cycle parameters can be given as options.
    for (size_t i=0; i<count("cycle"); i++) {
        std::string cycleParams = value("cycle", "", i);

        if (!parseCycleOptions(cycleParams)) return false;
    }

    // Smartcard number
    if (present("smartcard")) {

        // Whole smartcard reference
        std::string smartcardRef = value("smartcard");

        // Check length
        if (smartcardRef.length() != 12) {
            tsp->error("invalid length for smartcard option, exiting");
            return false;
        }

        // Smartcard number: 8 digits, indexes 3 to 10
        std::string smartcardNb = smartcardRef.substr(3, 8);

        // Remove zeros in front of smartcard number
        smartcardNb = smartcardNb.substr(smartcardNb.find_first_not_of('0'));

        // Convert to integer, exit on error
        bool status = ToInteger(_smartcard, smartcardNb.c_str());

        if (!status) {
            // Unable to correctly parse integer, exit
            tsp->error("bad format for smartcard option, exiting");
            return false;
        }

        // Max interval between two unique EMMs
        _uemm_interval = intValue<uint32_t>("emm_interval", (uint32_t) DEFAULT_UEMM_INTERVAL);

        // Set the max date of the next unique EMM (= now + interval)
        time_t now = time(NULL);
        _next_uemm_date = now + _uemm_interval;
    }
    else {
        _smartcard = 0;
        _uemm_interval = 0;
        _next_uemm_date = 0;
    }


    _emm_pids.reset();

    // Reinitialize the demux
    _demux.reset();
    _demux.addPID (PID_CAT);

    return true;
}



//----------------------------------------------------------------------------
// Parse a string with cycle options, and update the cycles' list.
// Cycle options are given as a string, with the following format:
// "cycleNumber-minPeriod-maxPeriod"
//----------------------------------------------------------------------------

bool ts::EMMMonitorPlugin::parseCycleOptions(const std::string& options)
{
    size_t index = 0;
    size_t lastIndex = 0;

    uint8_t  cycle = 0;
    uint16_t minPeriod = 0;
    uint16_t maxPeriod = 0;

    bool status = false;

    // Parse the given option
    index = options.find('-');

    if (index == std::string::npos) {
        // No '-' found, bad format, exit
        tsp->error("bad format for cycle option, exiting");
        return false;
    }

    // Cycle number
    std::string cycleNb = options.substr(lastIndex, index - lastIndex);

    // Convert to integer, exit on error
    status = ToInteger(cycle, cycleNb.c_str());

    if (!status) {
        // Unable to correctly parse integer, exit
        tsp->error("bad format for cycle option, exiting");
        return false;
    }

    lastIndex = index + 1;

    // Min cycle period
    index = options.find('-', lastIndex);

    if (index == std::string::npos) {
        // No '-' found, bad format, exit
        tsp->error("bad format for cycle option, exiting");
        return false;
    }
        
    std::string minPeriodStr = options.substr(lastIndex, index - lastIndex);

    // Convert to integer, exit on error
    status = ToInteger(minPeriod, minPeriodStr.c_str());

    if (!status) {
        // Unable to correctly parse integer, exit
        tsp->error("bad format for cycle option, exiting");
        return false;
    }

    lastIndex = index + 1;

    // Max cycle period
    std::string maxPeriodStr = options.substr(lastIndex, options.length() - lastIndex);

    // Convert to integer, exit on error
    status = ToInteger(maxPeriod, maxPeriodStr.c_str());

    if (!status) {
        // Unable to correctly parse integer, exit
        tsp->error("bad format for cycle option, exiting");
        return false;
    }

    // Check if min < max
    if (minPeriod >= maxPeriod) {
        tsp->error("bad format for cycle option, min > max period, exiting");
        return false;
    }

    // Update the cycles' list
    _cycle_params[cycle] = CycleParameters(0, minPeriod, maxPeriod);

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::EMMMonitorPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_CAT: {
            if (table.sourcePID() == PID_CAT) {
                CAT cat (table);
                if (cat.isValid()) {
                    processCAT (cat);
                }
            }
            break;
        }

        case TID_SA_EMM_T: {
            processTechnicalEMM (table);
            break;
        }

        case TID_SA_EMM_U: {
            // Handle unique EMMs only if a smartcard has been specified
            if (_smartcard != 0) {
                processUniqueEMM (table);
            }
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Conditional Access Table (CAT).
//----------------------------------------------------------------------------

void ts::EMMMonitorPlugin::processCAT (const CAT& cat)
{
    PIDOperatorSet pidop;

    // Add only SafeAccess EMM PIDs, checking PPID
    pidop.addSafeAccessCAT (cat.descs);
    addECMM (pidop, "EMM");
}


//----------------------------------------------------------------------------
// This method adds all ECM/EMM PIDs from the specified list if they match
// the optional selected CAS operator id.
//----------------------------------------------------------------------------

void ts::EMMMonitorPlugin::addECMM (const PIDOperatorSet& pidop, const char *name)
{
    for (PIDOperatorSet::const_iterator it = pidop.begin(); it != pidop.end(); ++it) {
        if (!_emm_pids[it->pid]) {
            tsp->verbose ("found %s PID %d (0x%04X)", name, int (it->pid), int (it->pid));

            _demux.addPID (it->pid);
            
            _emm_pids.set (it->pid);
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a SafeAccess Technical EMM
//----------------------------------------------------------------------------

void ts::EMMMonitorPlugin::processTechnicalEMM (const BinaryTable& table)
{
    const uint16_t MIN_PAYLOAD_SIZE = 9;
    const uint8_t  EMM_PARAM_STUFFING_TAG = 0xFF;
    const uint8_t  EMM_PARAM_CYCLE_TAG = 0x01;
//  const uint8_t  EMM_PARAM_MUX_CNX_TAG = 0x02;   // Not used yet

    if (table.sectionCount() != 1) {
        return;
    }


    // Check payload size
    if (table.sectionAt(0)->payloadSize() < MIN_PAYLOAD_SIZE) return;

    uint8_t firstByte = table.sectionAt(0)->payload()[6];

    // Ignore stuffing EMMs
    if (firstByte == EMM_PARAM_STUFFING_TAG) {
        return;
    }

    // Handle technical EMMs indicating cycle broadcast.
    if (firstByte == EMM_PARAM_CYCLE_TAG) {

        // Get the cycle number
        uint8_t cycle = table.sectionAt(0)->payload()[8];

        tsp->debug ("received technical EMM for cycle 0x%02X", cycle);

        time_t now = time (NULL);

        // If the map already contains an entry for this cycle, compute
        // the broadcast time, using the stored last broadcast date.
        if (_cycle_params.find(cycle) != _cycle_params.end()) {

            time_t lastBroadcast = _cycle_params[cycle].last_broadcast_date;
            uint16_t cyclePeriod;

            // Does not compute the cycle period if last date = 0 (in this case,
            // it means that we did not receive the technical EMM yet)
            if (lastBroadcast != 0) {
                cyclePeriod = (uint16_t) (now - lastBroadcast);

                tsp->verbose ("broadcast time for cycle 0x%02X = %d s", 
                    cycle, cyclePeriod);

                // Compare the computed cycle period with the allowed range.
                // A max period value equal to zero means no range.
                if (_cycle_params[cycle].max_period != 0) {
                    // Low bound
                    if (cyclePeriod < _cycle_params[cycle].min_period) {

                        std::string alarmMessage (Format ("broadcast time for cycle 0x%02X (%d s) is lower than allowed minimum (%d s)",
                            cycle, cyclePeriod, _cycle_params[cycle].min_period));

                        tsp->warning (alarmMessage);

                        // Call alarm script if defined, and pass the alarm message as parameter.
                        runAlarmCommand (alarmMessage);
                    }

                    // High bound
                    if (cyclePeriod > _cycle_params[cycle].max_period) {

                        std::string alarmMessage (Format ("broadcast time for cycle 0x%02X (%d s) is greater than allowed maximum (%d s)",
                            cycle, cyclePeriod, _cycle_params[cycle].max_period));

                        tsp->warning (alarmMessage);

                        // Call alarm script if defined, and pass the alarm message as parameter.
                        runAlarmCommand (alarmMessage);
                    }
                }  // if max period cycle is defined.
            }  // if last broadcast date is defined.

        }  // if a entry already exists in the list for this cycle.
        else {
            // No parameters for this cycle, add a new entry in the list
            _cycle_params[cycle] = CycleParameters();
        }

        // Store new broadcast time
        _cycle_params[cycle].last_broadcast_date = now;

    }

}


//----------------------------------------------------------------------------
//  This method processes a SafeAccess Unique EMM
//----------------------------------------------------------------------------

void ts::EMMMonitorPlugin::processUniqueEMM (const BinaryTable& table)
{
    const uint16_t MIN_PAYLOAD_SIZE = 6;

    // Check payload size
    if (table.sectionAt(0)->payloadSize() < MIN_PAYLOAD_SIZE) return;

    // Only handle EMMs which are addressed to the specified smartcard
    // Address is present in payload at indexes 2 to 5.
    uint32_t address = 0;
    address  = table.sectionAt(0)->payload()[2] << 24;
    address += table.sectionAt(0)->payload()[3] << 16;
    address += table.sectionAt(0)->payload()[4] << 8;
    address += table.sectionAt(0)->payload()[5];

    if (address != _smartcard) return;


    // We now have an unique EMM, addressed to the specified smartcard.
    // Check its content against the previous one.
    
    if (_uemm_section == *table.sectionAt(0)) {
        // Same EMM, nothing to do.
        return;
    }
    else {
        // The two EMMs are different. Store the newly received, and
        // update the max date.
        _uemm_section.copy(*table.sectionAt(0));

        _next_uemm_date = time (NULL) + _uemm_interval;
    }

}


//----------------------------------------------------------------------------
// Run the alarm command, if one was specified as the plugin option.
// The given string parameter describes the alarm.
//----------------------------------------------------------------------------

void ts::EMMMonitorPlugin::runAlarmCommand(const std::string& parameter) 
{
    // Do nothing if alarm command was not specified
    if (_alarm_command != "") {

        std::string completeCommand = _alarm_command + " " + '"' + parameter + '"';

        if (system(completeCommand.c_str()) != 0) {
            tsp->severe("unable to run alarm command %s", completeCommand.c_str());
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EMMMonitorPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _demux.feedPacket (pkt);


    // Check if the max date for the expected unique EMM
    // has been reached.
    if (_smartcard != 0) {  // only if a smartcard was specified
        time_t now = time(NULL);

        if (now > _next_uemm_date) {
            std::string alarmMessage (Format ("EMM for smartcard %u (0x%08X) has not been renewed during past %u seconds",
                _smartcard, _smartcard, _uemm_interval));

            tsp->warning (alarmMessage);

            // Call alarm script if defined, and pass the alarm message as parameter.
            runAlarmCommand (alarmMessage);

            // update max date, so that alarm is not repeated too often
            _next_uemm_date = now + _uemm_interval;
        }
    }

    // Pass all packets
    return TSP_OK;
}
