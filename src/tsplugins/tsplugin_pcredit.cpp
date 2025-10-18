//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Edit PCR, PTS and DTS values in various ways.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsNames.h"
#include "tsSystemRandomGenerator.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCREditPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCREditPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Type of units for PCR, PTS, DTS values.
        enum {UNIT_DEFAULT, UNIT_PCR, UNIT_PTS, UNIT_MILLISEC, UNIT_NANOSEC};

        // Command line options.
        bool    _ignore_scrambled = false;
        bool    _random = false;
        int64_t _add_pcr = 0;
        int64_t _add_pts = 0;
        int64_t _add_dts = 0;
        PIDSet  _pids {};
        SystemRandomGenerator _prng {};

        // Return actual value to apply.
        int64_t adjust(int64_t value);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcredit", ts::PCREditPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCREditPlugin::PCREditPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Edit PCR, PTS and DTS values in various ways", u"[options]")
{
    option(u"add-dts", 0, INT64);
    help(u"add-dts",
         u"Add the specified quantity to all DTS values (can be negative). "
         u"See options --unit and --random for the interpretation of the value.");

    option(u"add-pcr", 0, INT64);
    help(u"add-pcr",
         u"Add the specified quantity to all PCR values (can be negative). "
         u"See options --unit and --random for the interpretation of the value.");

    option(u"add-pts", 0, INT64);
    help(u"add-pts",
         u"Add the specified quantity to all PTS values (can be negative). "
         u"See options --unit and --random for the interpretation of the value.");

    option(u"ignore-scrambled", 'i');
    help(u"ignore-scrambled",
         u"Do not modify PCR values on PID's containing scrambled packets. "
         u"On scrambled PID's, only the PCR's can be modified. "
         u"The PTS and DTS are scrambled and cannot be edited.");

    option(u"negate-pids", 'n');
    help(u"negate-pids",
         u"Negate the selection of --pid options. "
         u"All PID's except the specified ones will have their timestamps edited.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specifies PID's where PCR, DTS and PTS values shall be edited. "
         u"By default, all PID's are modified. Several --pid options may be specified. ");

    option(u"random", 'r');
    help(u"random",
         u"The absolute values of --add-pcr, --add-dts, --add-pts are used as maximum values. "
         u"The added value is a random number in the range -n to +n where n is the absolute value of the corresponding parameter. "
         u"This option is typically used to intentionally corrupt time stamps.");

    option(u"unit", 'u', Names({
        {u"default",     UNIT_DEFAULT},
        {u"pcr",         UNIT_PCR},
        {u"pts",         UNIT_PTS},
        {u"dts",         UNIT_PTS},
        {u"millisecond", UNIT_MILLISEC},
        {u"nanosecond",  UNIT_NANOSEC},
    }));
    help(u"unit", u"name",
         u"Specify the unit of numeric values for options such as --add-pcr, --add-pts or --add-dts. "
         u"The default unit is \"default\", meaning that each value is a raw number to be applied "
         u"(--add-pcr value is in PCR units, --add-pts value is in PTS units, etc.) "
         u"Otherwise, it is possible to provide uniform values for all options in PCR units, "
         u"PTS/DTS units (the same), nanoseconds or milliseconds. "
         u"The specified values will be converted into the appropriate PCR or PTS/DTS units for each edited field.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PCREditPlugin::getOptions()
{
    _ignore_scrambled = present(u"ignore-scrambled");
    _random = present(u"random");
    getIntValue(_add_pcr, u"add-pcr", 0);
    getIntValue(_add_pts, u"add-pts", 0);
    getIntValue(_add_dts, u"add-dts", 0);
    getIntValues(_pids, u"pid", true);

    if (present(u"negate-pids")) {
        _pids.flip();
    }

    switch (intValue<int>(u"unit", UNIT_DEFAULT)) {
        case UNIT_PCR:
            _add_pts /= SYSTEM_CLOCK_SUBFACTOR;
            _add_dts /= SYSTEM_CLOCK_SUBFACTOR;
            break;
        case UNIT_PTS:
            _add_pcr *= SYSTEM_CLOCK_SUBFACTOR;
            break;
        case UNIT_MILLISEC:
            _add_pcr = cn::duration_cast<PCR>(cn::milliseconds(cn::milliseconds::rep(_add_pcr))).count();
            _add_pts = cn::duration_cast<PTS>(cn::milliseconds(cn::milliseconds::rep(_add_pts))).count();
            _add_dts = cn::duration_cast<DTS>(cn::milliseconds(cn::milliseconds::rep(_add_dts))).count();
            break;
        case UNIT_NANOSEC:
            _add_pcr = cn::duration_cast<PCR>(cn::nanoseconds(cn::nanoseconds::rep(_add_pcr))).count();
            _add_pts = cn::duration_cast<PTS>(cn::nanoseconds(cn::nanoseconds::rep(_add_pts))).count();
            _add_dts = cn::duration_cast<DTS>(cn::nanoseconds(cn::nanoseconds::rep(_add_dts))).count();
            break;
        default:
            break;
    }

    return true;
}


//----------------------------------------------------------------------------
// Return actual value to add to the time stamp.
//----------------------------------------------------------------------------

int64_t ts::PCREditPlugin::adjust(int64_t value)
{
    if (_random) {
        const int64_t max = std::abs(value);
        _prng.readInt(value, -max, max);
        debug(u"adjust by %+d", value);
    }
    return value;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCREditPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_pids.test(pkt.getPID())) {
        if (_ignore_scrambled && pkt.isScrambled()) {
            // First time we see a scrambled packet on this PID, exclude the PID.
            _pids.reset(pkt.getPID());
        }
        else {
            if (_add_pcr != 0 && pkt.hasPCR()) {
                pkt.setPCR(AddPCR(pkt.getPCR(), adjust(_add_pcr)));
            }
            if (_add_pts != 0 && pkt.hasPTS()) {
                pkt.setPTS((int64_t(pkt.getPTS()) + adjust(_add_pts)) & PTS_DTS_MASK);
            }
            if (_add_dts != 0 && pkt.hasDTS()) {
                pkt.setDTS((int64_t(pkt.getDTS()) + adjust(_add_dts)) & PTS_DTS_MASK);
            }
        }
    }
    return TSP_OK;
}
