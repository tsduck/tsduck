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
#include "tsAbstractStandardOutputStream.h"

namespace ts {
    //!
    //! Fork a process and create a pipe to its standard input, seen as a C++ std::basic_ostream<char>.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL ForkPipeOutputStream: public ForkPipe, public AbstractStandardOutputStream
    {
        TS_NOBUILD_NOCOPY(ForkPipeOutputStream);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit ForkPipeOutputStream(Report* report);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit ForkPipeOutputStream(ReporterBase* delegate);

        //!
        //! Destructor.
        //!
        virtual ~ForkPipeOutputStream() override;

        // Inherited from ForkPipe.
        virtual bool close(bool silent = false) override;

    protected:
        // Implementation of AbstractStandardOutputStream
        virtual bool writeStreamBuffer(const void* addr, size_t size) override;
    };
}
