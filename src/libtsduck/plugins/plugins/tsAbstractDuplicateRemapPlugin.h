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
        //!
        //! Constructor.
        //! @param [in] remap If true, use "remap" in help text, otherwise use "duplicate".
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        AbstractDuplicateRemapPlugin(bool remap, TSP* tsp, const UString& description = UString(), const UString& syntax = UString());

        // Implementation of plugin API
        virtual bool getOptions() override;

    protected:
        typedef std::map<PID, PID> PIDMap;  //!< A map from PID to PID.
        bool             _unchecked;        //!< Ignore conflicting input/output PID's.
        PIDSet           _newPIDs;          //!< Set of output (duplicated or remapped) PID values.
        PIDMap           _pidMap;           //!< Key = input pid, value = output PID.
        TSPacketLabelSet _setLabels;        //!< Labels to set on output packets.
        TSPacketLabelSet _resetLabels;      //!< Labels to reset on output packets.

    private:
        const bool _remap;
        // Strings for help and error messages:
        const UString _noun;
        const UString _verb;
        const UString _verbed;
        const UString _verbing;
    };
}
