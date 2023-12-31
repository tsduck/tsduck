//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSP.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSP::TSP(int max_severity) :
    Report(max_severity)
{
}

ts::TSP::~TSP()
{
}


//----------------------------------------------------------------------------
// Default implementations of virtual methods.
//----------------------------------------------------------------------------

bool ts::TSP::aborting() const
{
    return _tsp_aborting;
}
