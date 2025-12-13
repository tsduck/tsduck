//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a NetworkInformationFile (DVB-NIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteFile.h"

namespace ts::mcast {
    //!
    //! Representation of a NetworkInformationFile (DVB-NIP).
    //!
    //! Note: Currently, NetworkInformationFile is an alias for FluteFile and the XML
    //! content is not yet analyzed. Later, NetworkInformationFile should become a
    //! subclass of FluteFile, just like ServiceInformationFile.
    //!
    //! @see ETSI TS 103 876, section 8.4.2.2
    //! @see ServiceInformationFile
    //! @ingroup mpeg
    // To update when the class is defined: @ingroup libtsduck mpeg
    //!
    using NetworkInformationFile = FluteFile;
}
