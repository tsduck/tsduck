//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsMediaKey.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::hls::MediaKey::~MediaKey()
{
}


//----------------------------------------------------------------------------
// Check if the media segment is encrypted.
//----------------------------------------------------------------------------

bool ts::hls::MediaKey::isEncrypted() const
{
    return !method.empty() && !method.similar(u"NONE");
}


//----------------------------------------------------------------------------
// Check if the encryption key is in the default raw binary format.
//----------------------------------------------------------------------------

bool ts::hls::MediaKey::isRawEncryptionKey() const
{
    return format.empty() || format.similar(u"identity");
}
