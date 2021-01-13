//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!
//!  @file
//!  Version identification of TSDuck library interface.
//!
//----------------------------------------------------------------------------

#pragma once
//!
//! TSDuck library interface version.
//!
//! This version number is a simple integer value which is manually
//! incremented each time some incompatible change is introduced in the
//! TSDuck shared library interface.
//!
//! ## Rationale
//!
//! Users or third-party applications may provide TSDuck plugins or
//! extensions which are upgraded or installed independently from TSDuck.
//! If a plugin or extension is compiled with version @e N of the library
//! interface and TSDuck is upgraded to version @e N+1 (or vice-versa),
//! the plugin or extension binary will call binary features inside the
//! TSDuck library with the wrong API. Sometimes, the application will
//! not run (because symbols are missing for instance). In this case, the
//! problem is identified earlier and no problem occurs. But sometimes,
//! the change can be more subtle and won't be detected. This is the case,
//! for instance, when the plugin API is changed (new virtual methods).
//! On a link-edit perspective, all symbols are present, the third-party
//! plugin is registered successfully on startup but the application will
//! likely crash when the plugin is called because the structure of the
//! vtable of its class has changed.
//!
//! To avoid this, all registration features for plugins and extensions
//! which are usually automatically executed when third-party applications
//! or shared libraries are activated explicitly check this version number.
//! In case of mismatch, the registration is rejected.
//!
//! ## Maintenance guidelines
//!
//! This version number shall be manually incremented in the following case:
//!
//! - Incompatible modifications in the plugin interface. This means new or
//!   modified virtual methods or fields in the following classes:
//!   Report, Args, Plugin, InputPlugin, OutputPlugin, ProcessorPlugin
//!   and their subclasses.
//!
//! - Incompatible modifications in the table or descriptor interface.
//!
//! - Incompatible modifications in the TSDuck extension interface.
//!
#define TS_LIBRARY_VERSION 1
