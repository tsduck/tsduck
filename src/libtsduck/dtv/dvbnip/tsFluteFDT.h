//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the File Delivery Table (FDT) in the FLUTE protocol.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsFluteFile.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Representation of the File Delivery Table (FDT) in the FLUTE protocol.
    //! @see IETF RFC 3926, section 3.4.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteFDT : public FluteFile
    {
        TS_NOBUILD_NOCOPY(FluteFDT);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors.
        //! @param [in] source Source IP address.
        //! @param [in] destination Destination socket address.
        //! @param [in] tsi Transport Session Identifier.
        //! @param [in] instance_id FDT Instance ID.
        //! @param [in] content File content (XML text in UTF-8 representation).
        //!
        FluteFDT(Report&                report,
                 const IPAddress&       source,
                 const IPSocketAddress& destination,
                 uint64_t               tsi,
                 uint32_t               instance_id,
                 const ByteBlockPtr&    content);

        //!
        //! Check if the FDT was successfully parsed.
        //! @return True if the FDT was successfully parsed. False otherwise.
        //!
        bool isValid() const { return _valid; }

        //!
        //! Get the FDT Instance ID.
        //! @return The FDT Instance ID.
        //!
        uint32_t instanceId() const { return _instance_id; }

    private:
        bool     _valid = false;
        uint32_t _instance_id = 0;
    };
}
