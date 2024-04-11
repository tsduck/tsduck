//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard, Sergey Lobanov
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFuzzing.h"
#include "tsReport.h"
#include "tsDuckContext.h"
#include "tsSystemRandomGenerator.h"


//----------------------------------------------------------------------------
// Initialize the fuzzing operations.
//----------------------------------------------------------------------------

bool ts::TSFuzzing::start(const TSFuzzingArgs& options)
{
    _opt = options;
    _prng.reset();

    // If no fixed seed is provided, use a random one.
    if (_opt.seed.empty()) {
        SystemRandomGenerator sysrng;
        if (!sysrng.readByteBlock(_opt.seed, Xoshiro256ss::MIN_SEED_SIZE)) {
            _duck.report().error(u"system PRNG error");
            return false;
        }
        // Display the random seed in debug mode, for future reuse.
        if (_duck.report().debug()) {
            _duck.report().debug(u"fuzzing seed: %s", UString::Dump(_opt.seed, UString::COMPACT));
        }
    }

    // Reseed the PRNG until ready.
    for (size_t foolproof = Xoshiro256ss::MIN_SEED_SIZE; !_prng.ready() && foolproof > 0; --foolproof) {
        if (!_prng.seed(_opt.seed.data(), _opt.seed.size())) {
            _duck.report().error(u"error seeding reproducible PRNG");
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Process one packet from the stream.
//----------------------------------------------------------------------------

bool ts::TSFuzzing::processPacket(TSPacket& pkt)
{
    // Corrupt only packets from specified PID's.
    if (_opt.pids.test(pkt.getPID())) {

        // Current implemention: simple random corruption of any packet byte?
        for (size_t i = _opt.sync_byte ? 0 : 1; i < PKT_SIZE; ++i) {

            // Check if random value is less than probability
            if ((_prng.read64() % _opt.probability.denominator()) < _opt.probability.numerator()) {
                pkt.b[i] = uint8_t(_prng.read64());
            }
        }
    }

    return true;
}
