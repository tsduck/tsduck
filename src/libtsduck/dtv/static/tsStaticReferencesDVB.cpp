//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsStaticReferencesDVB.h"
#include "tsTables.h"

#include "tsDVBCharTableSingleByte.h"
#include "tsDVBCharTableUTF16.h"
#include "tsDVBCharTableUTF8.h"
#include "tsARIBCharset.h"
#include "tsDumpCharset.h"

#include "tsDropOutputPlugin.h"
#include "tsNullInputPlugin.h"
#include "tsFileInputPlugin.h"
#include "tsFileOutputPlugin.h"
#include "tsFilePacketPlugin.h"
#include "tsForkInputPlugin.h"
#include "tsForkOutputPlugin.h"
#include "tsForkPacketPlugin.h"
#include "tsIPInputPlugin.h"
#include "tsIPOutputPlugin.h"
#include "tsDektecInputPlugin.h"
#include "tsDektecOutputPlugin.h"
#include "tshlsInputPlugin.h"
#include "tshlsOutputPlugin.h"
#include "tsSRTInputPlugin.h"
#include "tsSRTOutputPlugin.h"
TSDUCK_SOURCE;

// Macros to generate a unique symbol name.
#define REF_NAME1(a,b) a##b
#define REF_NAME2(a,b) REF_NAME1(a,b)
#define REF_NAME       REF_NAME2(REF,__LINE__)

// Macros to generate a dummy reference to a type using a local unused instance.
#define REF_TYPE(type) static const ts::type REF_NAME; _refs.push_back(&REF_NAME)

// Macros to generate a dummy reference to a type using an existing instance.
#define REF_OBJECT(obj) _refs.push_back(&ts::obj)

//
// Constructor of the dummy reference-maker.
//
ts::StaticReferencesDVB::StaticReferencesDVB() :
    _refs()
{
    _refs.reserve(100);

    // References to all object files containing character sets.

    REF_OBJECT(DVBCharTableSingleByte::DVB_ISO_6937);
    REF_OBJECT(DVBCharTableUTF16::DVB_UNICODE);
    REF_OBJECT(DVBCharTableUTF8::DVB_UTF_8);
    REF_OBJECT(ARIBCharset::B24);
    REF_OBJECT(DumpCharset::DUMP);

    // Reference to some plugins which are statically registered.

    REF_OBJECT(DropOutputPlugin::REFERENCE);
    REF_OBJECT(NullInputPlugin::REFERENCE);
    REF_OBJECT(FileInputPlugin::REFERENCE);
    REF_OBJECT(FileOutputPlugin::REFERENCE);
    REF_OBJECT(FilePacketPlugin::REFERENCE);
    REF_OBJECT(ForkInputPlugin::REFERENCE);
    REF_OBJECT(ForkOutputPlugin::REFERENCE);
    REF_OBJECT(ForkPacketPlugin::REFERENCE);
    REF_OBJECT(IPInputPlugin::REFERENCE);
    REF_OBJECT(IPOutputPlugin::REFERENCE);
    REF_OBJECT(DektecInputPlugin::REFERENCE);
    REF_OBJECT(DektecOutputPlugin::REFERENCE);
    REF_OBJECT(hls::InputPlugin::REFERENCE);
    REF_OBJECT(hls::OutputPlugin::REFERENCE);
    REF_OBJECT(SRTInputPlugin::REFERENCE);
    REF_OBJECT(SRTOutputPlugin::REFERENCE);

    // References to all DVB tables and descriptors.
    // The file tsRefType.h is automatically generated.

    #include "private/tsRefType.h"
}
