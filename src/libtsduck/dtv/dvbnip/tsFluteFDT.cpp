//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteFDT.h"
#include "tsFlute.h"
#include "tsxmlDocument.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::FluteFDT::FluteFDT(Report&                report,
                       const IPAddress&       source,
                       const IPSocketAddress& destination,
                       uint64_t               tsi,
                       uint32_t               instance_id,
                       const ByteBlockPtr&    content_ptr) :
    FluteFile(source, destination, tsi, FLUTE_FDT_TOI, u"FDT", content_ptr),
    _valid(true),
    _instance_id(instance_id)
{
    // Parse the XML document.
    xml::Document doc(report);
    _valid = doc.parse(UString::FromUTF8(reinterpret_cast<const char*>(content().data()), size()));
    if (!_valid) {
        report.error(u"received an invalid FDT in TSI %d from %s", tsi, source);
        return;
    }

    //@@@
}
