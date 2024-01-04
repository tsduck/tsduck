//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard, Sergey Lobanov
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream packets fuzzing.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFuzzingArgs.h"
#include "tsTSPacket.h"
#include "tsXoshiro256ss.h"

namespace ts {
    //!
    //! Transport stream packets fuzzing.
    //! This class performs random corruptions on transport streams packets.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFuzzing
    {
        TS_NOBUILD_NOCOPY(TSFuzzing);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //!
        TSFuzzing(DuckContext& duck) : _duck(duck) {}

        //!
        //! Initialize the fuzzing operations.
        //! @param [in] options Fuzzing options.
        //! @return True on success, false on error.
        //!
        bool start(const TSFuzzingArgs& options);

        //!
        //! Process one packet from the stream.
        //! @param [in,out] pkt A TS packet from the stream. It may be corrupted on output, based on fuzzing options.
        //! @return True on success, false on error.
        //!
        bool processPacket(TSPacket& pkt);

    private:
        DuckContext&  _duck;
        TSFuzzingArgs _opt {};
        Xoshiro256ss  _prng {};
    };
}
