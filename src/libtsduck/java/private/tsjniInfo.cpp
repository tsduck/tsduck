//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//
//  Native implementation of the Java class io.tsduck.Info.
//
//----------------------------------------------------------------------------

#include "tsVersionInfo.h"
#include "tsVersionString.h"
#include "tsjni.h"

#if !defined(TS_NO_JAVA)

//
// public static native int intVersion();
//
TSDUCKJNI jint JNICALL Java_io_tsduck_Info_intVersion(JNIEnv* env, jclass clazz)
{
    return TS_VERSION_INTEGER;
}

//
// public static native String version();
//
TSDUCKJNI jstring JNICALL Java_io_tsduck_Info_version(JNIEnv* env, jclass clazz)
{
    return ts::jni::ToJString(env, ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT));
}

#endif // TS_NO_JAVA
