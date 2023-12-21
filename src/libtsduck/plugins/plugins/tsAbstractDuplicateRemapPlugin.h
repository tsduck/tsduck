//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::AbstractDuplicateRemapPlugin class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Abstract base class for the plugins @c duplicate and @c remap.
    //! This common base class defines the common options and their parsing.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractDuplicateRemapPlugin: public ProcessorPlugin
    {
        TS_NOCOPY(AbstractDuplicateRemapPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] remap If true, use "remap" in help text, otherwise use "duplicate".
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        AbstractDuplicateRemapPlugin(bool remap, TSP* tsp, const UString& description = UString(), const UString& syntax = UString());

        using PIDMap = std::map<PID, PID>;    //!< A map from PID to PID.
        bool             _unchecked = false;  //!< Ignore conflicting input/output PID's.
        PIDSet           _newPIDs {};         //!< Set of output (duplicated or remapped) PID values.
        PIDMap           _pidMap {};          //!< Key = input pid, value = output PID.
        TSPacketLabelSet _setLabels {};       //!< Labels to set on output packets.
        TSPacketLabelSet _resetLabels {};     //!< Labels to reset on output packets.

    private:
        const bool _remap;
        // Strings for help and error messages:
        const UString _noun;
        const UString _verb;
        const UString _verbed;
        const UString _verbing;
    };
}
