//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Fork a process the input of which is seen as a C++ std::basic_ostream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsForkPipe.h"
#include "tsAbstractOutputStream.h"

namespace ts {
    //!
    //! Fork a process and create a pipe to its standard input, seen as a C++ std::basic_ostream<char>.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL ForkPipeOutputStream: public ForkPipe, public AbstractOutputStream
    {
        TS_NOCOPY(ForkPipeOutputStream);
    public:
        //!
        //! Default constructor.
        //!
        ForkPipeOutputStream() = default;

        //!
        //! Destructor.
        //!
        virtual ~ForkPipeOutputStream() override;

        // Inherited from ForkPipe.
        virtual bool close(Report& report) override;

    protected:
        // Implementation of AbstractOutputStream
        virtual bool writeStreamBuffer(const void* addr, size_t size) override;
    };
}
