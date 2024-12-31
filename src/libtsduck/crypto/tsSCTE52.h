//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  ANSI/SCTE 52 DES-based TS packet encryption.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVS042.h"
#include "tsDES.h"

namespace ts {
    //!
    //! ANSI/SCTE 52 2003 DES-based TS packet encryption.
    //! @ingroup crypto
    //!
    //! DVS 042 has been renamed as "ANSI/SCTE 52 2003". The next iteration of
    //! this standard is "ANSI/SCTE 52 2008". The only difference between the
    //! two versions is the handling of messages shorter than the block size.
    //! In the 2003 (DVS 042) version, the same IV (called "whitener" in the
    //! standard) is used for long and short messages. In the 2008 version, a
    //! different "whitener2" must be used for messages shorter than the block size.
    //!
    class TSDUCKDLL SCTE52_2003 : public DVS042<DES>
    {
        TS_NOCOPY(SCTE52_2003);
    public:
        //! Constructor.
        SCTE52_2003();
        //! Destructor.
        virtual ~SCTE52_2003() override;

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();
    };

    //!
    //! ANSI/SCTE 52 2008 DES-based TS packet encryption.
    //! @ingroup crypto
    //!
    //! DVS 042 has been renamed as "ANSI/SCTE 52 2003". The next iteration of
    //! this standard is "ANSI/SCTE 52 2008". The only difference between the
    //! two versions is the handling of messages shorter than the block size.
    //! In the 2003 (DVS 042) version, the same IV (called "whitener" in the
    //! standard) is used for long and short messages. In the 2008 version, a
    //! different "whitener2" must be used for messages shorter than the block size.
    //!
    class TSDUCKDLL SCTE52_2008 : public DVS042<DES>
    {
        TS_NOCOPY(SCTE52_2008);
    public:
        //! Constructor.
        SCTE52_2008();
        //! Destructor.
        virtual ~SCTE52_2008() override;

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();
    };
}
