//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A set of DVB-T tuners parameters with an bitrate offset.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsModulationArgs.h"

namespace ts {

    class BitrateDifferenceDVBT;

    //!
    //! List of BitrateDifferenceDVBT.
    //!
    typedef std::list<BitrateDifferenceDVBT> BitrateDifferenceDVBTList;

    //!
    //! A variant of DVB-T tuners parameters with an offset between a target bitrate and their theoretical bitrate.
    //! @ingroup hardware
    //!
    class TSDUCKDLL BitrateDifferenceDVBT
    {
    public:
        ModulationArgs tune {};           //!< Modulation parameters.
        BitRate        bitrate_diff = 0;  //!< Difference between a target bitrate and the theoretial bitrate for these tuner parameters.

        //!
        //! Default constructor.
        //!
        BitrateDifferenceDVBT();

        //!
        //! Comparison operator for list sort.
        //! Sort criterion: increasing order of absolute value of bitrate_diff.
        //! This allow the creation of a list of parameters, from the closest to a target bitrate.
        //! @param [in] other Other instance to compare.
        //! @return True if this object is logically less than @a other.
        //!
        bool operator<(const BitrateDifferenceDVBT& other) const;

        //!
        //! Build a list of all possible combinations of DVB-T parameters for a target bitrate.
        //! @param [out] params List of all possible combinations of bandwidth, constellation,
        //! guard interval and high-priority FEC, sorted in increasing order of bitrate
        //! difference from a @a bitrate.
        //! @param [in] bitrate Target bitrate in bits/second.
        //!
        static void EvaluateToBitrate(BitrateDifferenceDVBTList& params, const BitRate& bitrate);
    };
}
