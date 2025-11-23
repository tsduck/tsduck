//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMulticastGatewayConfiguration.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::MulticastGatewayConfiguration::
    MulticastGatewayConfiguration(Report&               report,
                                  const FluteSessionId& sid,
                                  uint64_t              toi,
                                  const UString&        name,
                                  const UString&        type,
                                  const ByteBlockPtr&   content) :
    FluteFile(sid, toi, name, type, content)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"MulticastGatewayConfiguration", true)) {

        //@@@
        // const xml::Element* root = doc.rootElement();
    }
}


