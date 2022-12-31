//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//-----------------------------------------------------------------------------

#include "tsWinUtils.h"
#include "tsComPtr.h"
#include "tsSysUtils.h"
#include "tsRegistry.h"
#include "tsMemory.h"
#include "tsFatal.h"

#include "tsBeforeStandardHeaders.h"
#include <errors.h>
#include <shellapi.h>
#include <WinInet.h>
#include <dshowasf.h>
#include <ks.h>
#include <ksproxy.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include "tsAfterStandardHeaders.h"

// Required link libraries under Windows.
#if defined(TS_MSC)
    #pragma comment(lib, "Shell32.lib")
    #pragma comment(lib, "Wininet.lib")
    #pragma comment(lib, "quartz.lib")
#endif


//-----------------------------------------------------------------------------
// Convert Windows strings to UString (empty on error)
//-----------------------------------------------------------------------------

ts::UString ts::ToString(const ::VARIANT& var)
{
    return var.vt == ::VT_BSTR ? ToString(var.bstrVal) : UString();
}

ts::UString ts::ToString(const ::BSTR bstr)
{
    assert(sizeof(*bstr) == sizeof(ts::UChar));
    return UString(reinterpret_cast<const UChar*>(bstr));
}

ts::UString ts::ToString(const ::WCHAR* str)
{
    assert(sizeof(*str) == sizeof(ts::UChar));
    return UString(reinterpret_cast<const UChar*>(str));
}


//-----------------------------------------------------------------------------
// Format a Windows error message (Windows-specific).
//-----------------------------------------------------------------------------

ts::UString ts::WinErrorMessage(::DWORD code, const UString& moduleName, ::DWORD minModuleCode, ::DWORD maxModuleCode)
{
    UString message;

    // Start with module-specific error codes.
    if (!moduleName.empty() && code >= minModuleCode && code <= maxModuleCode) {
        // Get a handle to the module. Fail if the module is not loaded in memory.
        // This kind of handle does not need to be closed.
        const ::HMODULE hmod = ::GetModuleHandleW(moduleName.wc_str());
        if (hmod != 0) {
            message.resize(1024);
            ::DWORD length = ::FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, hmod, code, 0, message.wc_str(), ::DWORD(message.size()), NULL);
            message.trimLength(length, true);
        }
    }

    // If no message was found from a specific module, search in the system base.
    if (message.empty()) {
        message.resize(1024);
        ::DWORD length = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, message.wc_str(), ::DWORD(message.size()), NULL);
        message.trimLength(length, true);
    }

    // Get additional information for some special code.
    if (code == ERROR_INTERNET_EXTENDED_ERROR) {
        ::DWORD code2 = 0;
        ::DWORD length = 0;
        // First call without output buffer, to get the required size.
        ::InternetGetLastResponseInfoW(&code2, 0, &length);
        if (length > 0) {
            // Now, we know the required size. Retry with a correctly-sized buffer.
            UString info(size_t(length), CHAR_NULL);
            if (::InternetGetLastResponseInfoW(&code2, info.wc_str(), &length)) {
                // Got an extended message, append to previous message.
                info.trimLength(length, true);
                if (!message.empty()) {
                    message.append(u", ");
                }
                message.append(info);
            }
        }
    }

    // If no message is found, return a generic message.
    return message.empty() ? UString::Format(u"System error %d (0x%X)", {code, code}) : message;
}


//-----------------------------------------------------------------------------
// Get the device or file name from a Windows handle
//-----------------------------------------------------------------------------

ts::UString ts::WinDeviceName(::HANDLE handle)
{
    // First, try with GetFinalPathNameByHandle.
    // This works fine for files but not for named pipes.
    std::array<::WCHAR, 2048> name_buffer;
    ::DWORD size = ::GetFinalPathNameByHandleW(handle, name_buffer.data(), ::DWORD(name_buffer.size() - 1), FILE_NAME_NORMALIZED);
    size = std::max(::DWORD(0), std::min(size, ::DWORD(name_buffer.size() - 1)));

    // If a non-empty name was found, use it.
    if (size > 0) {
        // Convert to a UString.
        UString name(name_buffer, size);
        // Remove useless prefix \\?\ if present.
        if (name.startWith(u"\\\\?\\")) {
            name.erase(0, 4);
        }
        return name;
    }

    // Could not find a useful name with GetFinalPathNameByHandle.
    // Try GetFileInformationByHandleEx (which uses an untyped buffer).
    uint8_t buf[2048];
    ::memset(buf, 0, sizeof(buf));

    // With FileNameInfo, the buffer is a FILE_NAME_INFO structure.
    PFILE_NAME_INFO info = PFILE_NAME_INFO(&buf);

    // Maximum number of WCHAR in the FileName field of the FILE_NAME_INFO.
    const size_t max_wchar = (buf + sizeof(buf) - (uint8_t*)(info->FileName)) / sizeof(::WCHAR);

    if (!::GetFileInformationByHandleEx(handle, ::FileNameInfo, buf, sizeof(buf))) {
        return UString(); // error
    }
    else {
        info->FileName[std::min<size_t>(max_wchar - 1, info->FileNameLength)] = 0;
        return ToString(info->FileName);
    }
}


//-----------------------------------------------------------------------------
// Start an application with elevated privileges (Windows-specific).
//-----------------------------------------------------------------------------

bool ts::WinCreateElevatedProcess(const UString& exeName, bool synchronous, Report& report)
{
    ::SHELLEXECUTEINFOW info;
    TS_ZERO(info);
    info.cbSize = sizeof(info);

    info.fMask = synchronous ? SEE_MASK_NOCLOSEPROCESS : SEE_MASK_DEFAULT;
    info.lpVerb = L"runas";
    info.lpFile = exeName.wc_str();
    info.lpParameters = L"";
    info.nShow = SW_SHOW;

    if (!::ShellExecuteExW(&info)) {
        report.error(u"error starting %s: %s", {exeName, WinErrorMessage(::GetLastError())});
        return false;
    }

    // Wait for process termination.
    if (synchronous) {
        ::WaitForSingleObject(info.hProcess, INFINITE);
        ::CloseHandle(info.hProcess);
    }
    return true;
}


//-----------------------------------------------------------------------------
// Format the message for a COM status
//-----------------------------------------------------------------------------

ts::UString ts::ComMessage(::HRESULT hr)
{
    // Get DirectShow error message
    std::array<::WCHAR, MAX_ERROR_TEXT_LEN> buf;
    ::DWORD size = ::AMGetErrorTextW(hr, buf.data(), ::DWORD(buf.size()));
    size = std::max(::DWORD(0), std::min(size, ::DWORD(buf.size() - 1)));
    buf[size] = 0;

    // Remove trailing newlines (if any)
    while (size > 0 && (buf[size-1] == u'\n' || buf[size-1] == u'\r')) {
        buf[--size] = 0;
    }

    // If DirectShow message is empty, use Win32 error message
    if (size > 0) {
        return UString(buf, size);
    }
    else {
        return SysErrorCodeMessage(hr);
    }
}


//-----------------------------------------------------------------------------
// Check a COM status. In case of error, report an error message.
// Return true is status is success, false if error.
//-----------------------------------------------------------------------------

bool ts::ComSuccess(::HRESULT hr, const UString& message, Report& report)
{
    return ComSuccess(hr, message.c_str(), report);
}


bool ts::ComSuccess(::HRESULT hr, const UChar* message, Report& report)
{
    if (SUCCEEDED(hr)) {
        if (message != nullptr && report.maxSeverity() >= 10) {
            report.log(10, u"%s: success", {message});
        }
        return true;
    }
    else {
        report.error(u"%s: %s", {message != nullptr ? message : u"COM error", ComMessage(hr)});
        return false;
    }
}


//-----------------------------------------------------------------------------
// Check if an object exposes an interface.
//-----------------------------------------------------------------------------

bool ts::ComExpose(::IUnknown* object, const ::IID& iid)
{
    ::IUnknown* iface;
    if (object != 0 && SUCCEEDED(object->QueryInterface(iid, (void**)&iface))) {
        iface->Release();
        return true;
    }
    else {
        return false;
    }
}


//-----------------------------------------------------------------------------
// Get the handle of a COM object.
//-----------------------------------------------------------------------------

// WARNING: We are doing something weird here...
// The IKsObject interface is supposedly declared in ksproxy.h.
// However, the declaraction is not inlined, unless the Windows driver
// development kit is installed and Streams.h included. We do not want
// to require the DDK to be installed in order to compile TSDuck. This
// is why it is redeclared here. However, in case of incorrect declaraction,
// you may expect a crash....
MIDL_INTERFACE("423c13a2-2070-11d0-9ef7-00aa00a216a1")
IKsObject : public IUnknown
{
public:
    virtual HANDLE STDMETHODCALLTYPE KsGetObjectHandle();
};

::HANDLE ts::GetHandleFromObject(::IUnknown* obj, Report& report)
{
    // Query IKsObject interface on the object.
    ComPtr<::IKsObject> ks;
    report.log(2, u"WinUtils.GetHandleFromObject: getting IKsObject interface");
    ks.queryInterface(obj, IID_IKsObject, report);
    if (ks.isNull()) {
        return INVALID_HANDLE_VALUE;
    }

    // Return the handle. Note that KsGetObjectHandle returns zero on error, not INVALID_HANDLE_VALUE.
    report.log(2, u"WinUtils.GetHandleFromObject: IKsObject found, calling KsGetObjectHandle");
    const ::HANDLE h = ks->KsGetObjectHandle();
    report.log(2, u"WinUtils.GetHandleFromObject: handle: 0x%X", {uintptr_t(h)});
    return h == 0 ? INVALID_HANDLE_VALUE : h;
}


//-----------------------------------------------------------------------------
// Return a string property from the "property bag" of an object
// (defined by an object moniker)
//-----------------------------------------------------------------------------

ts::UString ts::GetStringPropertyBag(::IMoniker* object_moniker, const ::OLECHAR* property_name, Report& report)
{
    // Bind to the object's storage, get the "property bag" interface
    ComPtr <::IPropertyBag> pbag;
    ::HRESULT hr = object_moniker->BindToStorage(0,                       // No cached context
                                                 0,                       // Not part of a composite
                                                 ::IID_IPropertyBag,      // ID of requested interface
                                                 (void**)pbag.creator()); // Returned interface
    if (!ComSuccess(hr, u"IMoniker::BindToStorage", report)) {
        return UString();
    }

    // Get property from property bag

    UString value;
    ::VARIANT var;
    ::VariantInit(&var);
    hr = pbag->Read(property_name, &var, 0);
    if (ComSuccess(hr, u"IPropertyBag::Read", report)) {
        value = ToString(var);
    }
    ::VariantClear(&var);

    return value;
}


//-----------------------------------------------------------------------------
// Format a GUID as string (Windows-specific).
//-----------------------------------------------------------------------------

ts::UString ts::FormatGUID(const ::GUID& guid, bool with_braces)
{
    const UString s(UString::Format(u"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                                    {guid.Data1, guid.Data2, guid.Data3,
                                     guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                                     guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]}));
    return with_braces ? u"{" + s + u"}" : s;
}


//-----------------------------------------------------------------------------
// Get a "canonical" version of a GUID string (Windows-specific).
//-----------------------------------------------------------------------------

ts::UString ts::CanonicalGUID(const ::GUID& guid)
{
    return UString::Format(u"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                           {guid.Data1, guid.Data2, guid.Data3,
                            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]});
}

ts::UString ts::CanonicalGUID(const UString& guid)
{
    UString result;
    for (size_t i = 0; i < guid.length(); ++i) {
        if (IsHexa(guid[i])) {
            result.append(ToLower(guid[i]));
        }
    }
    return result;
}


//-----------------------------------------------------------------------------
// Format the name of a GUID. Resolve a few known names
//-----------------------------------------------------------------------------

ts::UString ts::NameGUID(const ::GUID& guid)
{
    // Build default formattings as found in the registry.
    const UString fmt0(FormatGUID(guid, false));
    const UString fmt(u"{" + fmt0 + u"}");
    const UString fmt1(fmt0.toLower());
    const UString fmt2(fmt.toLower());

    // Storage of GUID's in the registry.
    struct RegistryLocation {
        const UChar* key;
        const UChar* prefix;
    };
    static const RegistryLocation registryLocations[] = {
        // Windows XP style
        {u"HKEY_CLASSES_ROOT\\CLSID\\", u"CLSID_"},
        {u"HKEY_CLASSES_ROOT\\Interface\\", u"IID_"},
        {u"HKEY_CLASSES_ROOT\\DirectShow\\MediaObjects\\", u"DirectShow.MediaObject:"},
        {u"HKEY_CLASSES_ROOT\\DirectShow\\MediaObjects\\Categories\\", u"DirectShow.MediaObject.Category:"},
        {u"HKEY_CLASSES_ROOT\\Filter\\", u"Filter:"},
        {u"HKEY_CLASSES_ROOT\\CLSID\\{DA4E3DA0-D07D-11d0-BD50-00A0C911CE86}\\Instance\\", u"ActiveMovie.FilterCategories:"},
        // Windows 7 and 10 style
        {u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\", u"CLSID_"},
        {u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Interface\\", u"IID_"},
        {u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\DirectShow\\MediaObjects\\", u"DirectShow.MediaObject:"},
        {u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\DirectShow\\MediaObjects\\Categories\\", u"DirectShow.MediaObject.Category:"},
        {u"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\", u"System.Class:"},
        {u"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\MediaCategories\\", u"System.MediaCategory:"},
        {u"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\MediaInterfaces\\", u"System.MediaInterfaces:"},
        {0, 0}
    };

    // Check if the GUID is stored in the registry.
    for (const RegistryLocation* p = registryLocations; p->key != 0; ++p) {
        UString name;
        if (!(name = Registry::GetValue(p->key + fmt)).empty() ||
            !(name = Registry::GetValue(p->key + fmt0)).empty() ||
            !(name = Registry::GetValue(p->key + fmt1)).empty() ||
            !(name = Registry::GetValue(p->key + fmt2)).empty())
        {
            return p->prefix + name;
        }
    }

    // Check some predefined GUID values
    struct KnownValue {
        const ::GUID* id;
        const UChar*   name;
    };
    static const KnownValue knownValues[] = {
#define _N_(g) {&g, u#g},
        _N_(AM_INTERFACESETID_Standard)
        _N_(AM_KSCATEGORY_AUDIO)
        _N_(AM_KSCATEGORY_CAPTURE)
        _N_(AM_KSCATEGORY_CROSSBAR)
        _N_(AM_KSCATEGORY_DATACOMPRESSOR)
        _N_(AM_KSCATEGORY_RENDER)
        _N_(AM_KSCATEGORY_SPLITTER)
        _N_(AM_KSCATEGORY_TVAUDIO)
        _N_(AM_KSCATEGORY_TVTUNER)
        _N_(AM_KSCATEGORY_VBICODEC)
        _N_(AM_KSCATEGORY_VIDEO)
        _N_(AM_KSPROPSETID_AC3)
        _N_(AM_KSPROPSETID_CopyProt)
        _N_(AM_KSPROPSETID_DVD_RateChange)
        _N_(AM_KSPROPSETID_DvdKaraoke)
        _N_(AM_KSPROPSETID_DvdSubPic)
        _N_(AM_KSPROPSETID_FrameStep)
        _N_(AM_KSPROPSETID_TSRateChange)
        _N_(AMPROPSETID_Pin)
        _N_(ANALOG_AUXIN_NETWORK_TYPE)
        _N_(ANALOG_FM_NETWORK_TYPE)
        _N_(ANALOG_TV_NETWORK_TYPE)
        _N_(ATSC_TERRESTRIAL_TV_NETWORK_TYPE)
        _N_(CLSID_Proxy)
        _N_(CLSID_SinkFilter) // TSDuck specific.
        _N_(DIGITAL_CABLE_NETWORK_TYPE)
        _N_(DIRECT_TV_SATELLITE_TV_NETWORK_TYPE)
        _N_(DVB_CABLE_TV_NETWORK_TYPE)
        _N_(DVB_SATELLITE_TV_NETWORK_TYPE)
        _N_(DVB_TERRESTRIAL_TV_NETWORK_TYPE)
        _N_(EVENTID_CADenialCountChanged)
        _N_(EVENTID_CardStatusChanged)
        _N_(EVENTID_DRMParingStatusChanged)
        _N_(EVENTID_EASMessageReceived)
        _N_(EVENTID_EntitlementChanged)
        _N_(EVENTID_MMIMessage)
        _N_(EVENTID_NewSignalAcquired)
        _N_(EVENTID_PSITable)
        _N_(EVENTID_SignalStatusChanged)
        _N_(EVENTID_STBChannelNumber)
        _N_(EVENTID_TuningChanged)
        _N_(EVENTID_TuningChanging)
        _N_(FORMAT_525WSS)
        _N_(FORMAT_AnalogVideo)
        _N_(FORMAT_DolbyAC3)
        _N_(FORMAT_DVD_LPCMAudio)
        _N_(FORMAT_DvInfo)
        _N_(FORMAT_MPEG2_VIDEO)
        _N_(FORMAT_MPEG2Audio)
        _N_(FORMAT_MPEG2Video)
        _N_(FORMAT_MPEGStreams)
        _N_(FORMAT_MPEGVideo)
        _N_(FORMAT_None)
        _N_(FORMAT_VideoInfo)
        _N_(FORMAT_VideoInfo2)
        _N_(FORMAT_VIDEOINFO2)
        _N_(FORMAT_WaveFormatEx)
        _N_(GUID_NULL)
        _N_(IID_IAMAnalogVideoDecoder)
        _N_(IID_IAMAnalogVideoEncoder)
        _N_(IID_IAMAudioInputMixer)
        _N_(IID_IAMAudioRendererStats)
        _N_(IID_IAMBufferNegotiation)
        _N_(IID_IAMCameraControl)
        _N_(IID_IAMCertifiedOutputProtection)
        _N_(IID_IAMClockAdjust)
        _N_(IID_IAMClockSlave)
        _N_(IID_IAMCopyCaptureFileProgress)
        _N_(IID_IAMCrossbar)
        _N_(IID_IAMDecoderCaps)
        _N_(IID_IAMDeviceRemoval)
        _N_(IID_IAMDevMemoryAllocator)
        _N_(IID_IAMDevMemoryControl)
        _N_(IID_IAMDroppedFrames)
        _N_(IID_IAMErrorLog)
        _N_(IID_IAMExtDevice)
        _N_(IID_IAMExtTransport)
        _N_(IID_IAMFilterMiscFlags)
        _N_(IID_IAMGraphBuilderCallback)
        _N_(IID_IAMGraphStreams)
        _N_(IID_IAMLatency)
        _N_(IID_IAMMediaStream)
        _N_(IID_IAMMediaTypeSample)
        _N_(IID_IAMMediaTypeStream)
        _N_(IID_IAMMultiMediaStream)
        _N_(IID_IAMOpenProgress)
        _N_(IID_IAMOverlayFX)
        _N_(IID_IAMovieSetup)
        _N_(IID_IAMPhysicalPinInfo)
        _N_(IID_IAMPushSource)
        _N_(IID_IAMResourceControl)
        _N_(IID_IAMSetErrorLog)
        _N_(IID_IAMStreamConfig)
        _N_(IID_IAMStreamControl)
        _N_(IID_IAMStreamSelect)
        _N_(IID_IAMTimecodeDisplay)
        _N_(IID_IAMTimecodeGenerator)
        _N_(IID_IAMTimecodeReader)
        _N_(IID_IAMTimeline)
        _N_(IID_IAMTimelineComp)
        _N_(IID_IAMTimelineEffect)
        _N_(IID_IAMTimelineEffectable)
        _N_(IID_IAMTimelineGroup)
        _N_(IID_IAMTimelineObj)
        _N_(IID_IAMTimelineSplittable)
        _N_(IID_IAMTimelineSrc)
        _N_(IID_IAMTimelineTrack)
        _N_(IID_IAMTimelineTrans)
        _N_(IID_IAMTimelineTransable)
        _N_(IID_IAMTimelineVirtualTrack)
        _N_(IID_IAMTuner)
        _N_(IID_IAMTunerNotification)
        _N_(IID_IAMTVAudio)
        _N_(IID_IAMTVAudioNotification)
        _N_(IID_IAMTVTuner)
        _N_(IID_IAMVfwCaptureDialogs)
        _N_(IID_IAMVfwCompressDialogs)
        _N_(IID_IAMVideoAccelerator)
        _N_(IID_IAMVideoAcceleratorNotify)
        _N_(IID_IAMVideoCompression)
        _N_(IID_IAMVideoControl)
        _N_(IID_IAMVideoDecimationProperties)
        _N_(IID_IAMVideoProcAmp)
        _N_(IID_IAMWMBufferPass)
        _N_(IID_IAMWMBufferPassCallback)
        _N_(IID_IBDA_AutoDemodulate)
        _N_(IID_IBDA_AutoDemodulateEx)
        _N_(IID_IBDA_ConditionalAccess)
        _N_(IID_IBDA_DeviceControl)
        _N_(IID_IBDA_DiagnosticProperties)
        _N_(IID_IBDA_DigitalDemodulator)
        _N_(IID_IBDA_DRM)
        _N_(IID_IBDA_EasMessage)
        _N_(IID_IBDA_EthernetFilter)
        _N_(IID_IBDA_FrequencyFilter)
        _N_(IID_IBDA_IPSinkControl)
        _N_(IID_IBDA_IPSinkInfo)
        _N_(IID_IBDA_IPV4Filter)
        _N_(IID_IBDA_IPV6Filter)
        _N_(IID_IBDA_LNBInfo)
        _N_(IID_IBDA_NetworkProvider)
        _N_(IID_IBDA_NullTransform)
        _N_(IID_IBDA_PinControl)
        _N_(IID_IBDA_SignalProperties)
        _N_(IID_IBDA_SignalStatistics)
        _N_(IID_IBDA_TIF_REGISTRATION)
        _N_(IID_IBDA_Topology)
        _N_(IID_IBDA_TransportStreamInfo)
        _N_(IID_IBDA_VoidTransform)
        _N_(IID_IBDAComparable)
        _N_(IID_IKsAggregateControl)
        _N_(IID_IKsAllocator)
        _N_(IID_IKsAllocatorEx)
        _N_(IID_IKsClockPropertySet)
        _N_(IID_IKsControl)
        _N_(IID_IKsDataTypeCompletion)
        _N_(IID_IKsDataTypeHandler)
        _N_(IID_IKsInterfaceHandler)
        _N_(IID_IKsObject)
        _N_(IID_IKsPin)
        _N_(IID_IKsPinEx)
        _N_(IID_IKsPinFactory)
        _N_(IID_IKsPinPipe)
        _N_(IID_IKsPropertySet)
        _N_(IID_IKsTopology)
        _N_(ISDB_CABLE_TV_NETWORK_TYPE)
        _N_(ISDB_SATELLITE_TV_NETWORK_TYPE)
        _N_(ISDB_TERRESTRIAL_TV_NETWORK_TYPE)
        _N_(KSCATEGORY_BDA_NETWORK_EPG)
        _N_(KSCATEGORY_BDA_NETWORK_PROVIDER)
        _N_(KSCATEGORY_BDA_NETWORK_TUNER)
        _N_(KSCATEGORY_BDA_RECEIVER_COMPONENT)
        _N_(KSCATEGORY_BDA_TRANSPORT_INFORMATION)
        _N_(KSCATEGORY_IP_SINK)
        _N_(KSDATAFORMAT_SPECIFIER_BDA_IP)
        _N_(KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT)
        _N_(KSDATAFORMAT_SUBTYPE_ATSC_SI)
        _N_(KSDATAFORMAT_SUBTYPE_BDA_IP)
        _N_(KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL)
        _N_(KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT)
        _N_(KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_OOB_PSIP)
        _N_(KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP)
        _N_(KSDATAFORMAT_SUBTYPE_DVB_SI)
        _N_(KSDATAFORMAT_TYPE_BDA_ANTENNA)
        _N_(KSDATAFORMAT_TYPE_BDA_IF_SIGNAL)
        _N_(KSDATAFORMAT_TYPE_BDA_IP)
        _N_(KSDATAFORMAT_TYPE_BDA_IP_CONTROL)
        _N_(KSDATAFORMAT_TYPE_MPE)
        _N_(KSDATAFORMAT_TYPE_MPEG2_SECTIONS)
        _N_(KSEVENTSETID_BdaCAEvent)
        _N_(KSEVENTSETID_BdaPinEvent)
        _N_(KSEVENTSETID_BdaTunerEvent)
        _N_(KSMETHODSETID_BdaChangeSync)
        _N_(KSMETHODSETID_BdaDeviceConfiguration)
        _N_(KSNODE_BDA_8PSK_DEMODULATOR)
        _N_(KSNODE_BDA_8VSB_DEMODULATOR)
        _N_(KSNODE_BDA_ANALOG_DEMODULATOR)
        _N_(KSNODE_BDA_COFDM_DEMODULATOR)
        _N_(KSNODE_BDA_COMMON_CA_POD)
        _N_(KSNODE_BDA_OPENCABLE_POD)
        _N_(KSNODE_BDA_PID_FILTER)
        _N_(KSNODE_BDA_QAM_DEMODULATOR)
        _N_(KSNODE_BDA_QPSK_DEMODULATOR)
        _N_(KSNODE_BDA_RF_TUNER)
        _N_(KSNODE_BDA_VIDEO_ENCODER)
        _N_(KSNODE_IP_SINK)
        _N_(KSPROPSETID_BdaAutodemodulate)
        _N_(KSPROPSETID_BdaCA)
        _N_(KSPROPSETID_BdaDigitalDemodulator)
        _N_(KSPROPSETID_BdaEthernetFilter)
        _N_(KSPROPSETID_BdaFrequencyFilter)
        _N_(KSPROPSETID_BdaIPv4Filter)
        _N_(KSPROPSETID_BdaIPv6Filter)
        _N_(KSPROPSETID_BdaLNBInfo)
        _N_(KSPROPSETID_BdaNullTransform)
        _N_(KSPROPSETID_BdaPIDFilter)
        _N_(KSPROPSETID_BdaPinControl)
        _N_(KSPROPSETID_BdaSignalStats)
        _N_(KSPROPSETID_BdaTableSection)
        _N_(KSPROPSETID_BdaTopology)
        _N_(KSPROPSETID_BdaVoidTransform)
        _N_(LOOK_DOWNSTREAM_ONLY)
        _N_(LOOK_UPSTREAM_ONLY)
        _N_(MEDIASUBTYPE_708_608Data)
        _N_(MEDIASUBTYPE_A2B10G10R10)
        _N_(MEDIASUBTYPE_A2R10G10B10)
        _N_(MEDIASUBTYPE_AI44)
        _N_(MEDIASUBTYPE_AIFF)
        _N_(MEDIASUBTYPE_AnalogVideo_NTSC_M)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_B)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_D)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_G)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_H)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_I)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_M)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_N)
        _N_(MEDIASUBTYPE_AnalogVideo_PAL_N_COMBO)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_B)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_D)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_G)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_H)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_K)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_K1)
        _N_(MEDIASUBTYPE_AnalogVideo_SECAM_L)
        _N_(MEDIASUBTYPE_ARGB1555)
        _N_(MEDIASUBTYPE_ARGB1555_D3D_DX7_RT)
        _N_(MEDIASUBTYPE_ARGB1555_D3D_DX9_RT)
        _N_(MEDIASUBTYPE_ARGB32)
        _N_(MEDIASUBTYPE_ARGB32_D3D_DX7_RT)
        _N_(MEDIASUBTYPE_ARGB32_D3D_DX9_RT)
        _N_(MEDIASUBTYPE_ARGB4444)
        _N_(MEDIASUBTYPE_ARGB4444_D3D_DX7_RT)
        _N_(MEDIASUBTYPE_ARGB4444_D3D_DX9_RT)
        _N_(MEDIASUBTYPE_Asf)
        _N_(MEDIASUBTYPE_ATSC_SI)
        _N_(MEDIASUBTYPE_AU)
        _N_(MEDIASUBTYPE_Avi)
        _N_(MEDIASUBTYPE_AYUV)
        _N_(MEDIASUBTYPE_CFCC)
        _N_(MEDIASUBTYPE_CLJR)
        _N_(MEDIASUBTYPE_CLPL)
        _N_(MEDIASUBTYPE_CPLA)
        _N_(MEDIASUBTYPE_DOLBY_AC3)
        _N_(MEDIASUBTYPE_DOLBY_AC3_SPDIF)
        _N_(MEDIASUBTYPE_DRM_Audio)
        _N_(MEDIASUBTYPE_DssAudio)
        _N_(MEDIASUBTYPE_DssVideo)
        _N_(MEDIASUBTYPE_DTS)
        _N_(MEDIASUBTYPE_DtvCcData)
        _N_(MEDIASUBTYPE_dv25)
        _N_(MEDIASUBTYPE_dv50)
        _N_(MEDIASUBTYPE_DVB_SI)
        _N_(MEDIASUBTYPE_DVCS)
        _N_(MEDIASUBTYPE_DVD_LPCM_AUDIO)
        _N_(MEDIASUBTYPE_DVD_NAVIGATION_DSI)
        _N_(MEDIASUBTYPE_DVD_NAVIGATION_PCI)
        _N_(MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER)
        _N_(MEDIASUBTYPE_DVD_SUBPICTURE)
        _N_(MEDIASUBTYPE_dvh1)
        _N_(MEDIASUBTYPE_dvhd)
        _N_(MEDIASUBTYPE_dvsd)
        _N_(MEDIASUBTYPE_DVSD)
        _N_(MEDIASUBTYPE_dvsl)
        _N_(MEDIASUBTYPE_H264)
        _N_(MEDIASUBTYPE_IA44)
        _N_(MEDIASUBTYPE_IEEE_FLOAT)
        _N_(MEDIASUBTYPE_IF09)
        _N_(MEDIASUBTYPE_IJPG)
        _N_(MEDIASUBTYPE_IMC1)
        _N_(MEDIASUBTYPE_IMC2)
        _N_(MEDIASUBTYPE_IMC3)
        _N_(MEDIASUBTYPE_IMC4)
        _N_(MEDIASUBTYPE_IYUV)
        _N_(MEDIASUBTYPE_Line21_BytePair)
        _N_(MEDIASUBTYPE_Line21_GOPPacket)
        _N_(MEDIASUBTYPE_Line21_VBIRawData)
        _N_(MEDIASUBTYPE_MDVF)
        _N_(MEDIASUBTYPE_MJPG)
        _N_(MEDIASUBTYPE_MPEG1Audio)
        _N_(MEDIASUBTYPE_MPEG1AudioPayload)
        _N_(MEDIASUBTYPE_MPEG1Packet)
        _N_(MEDIASUBTYPE_MPEG1Payload)
        _N_(MEDIASUBTYPE_MPEG1System)
        _N_(MEDIASUBTYPE_MPEG1Video)
        _N_(MEDIASUBTYPE_MPEG1VideoCD)
        _N_(MEDIASUBTYPE_MPEG2_AUDIO)
        _N_(MEDIASUBTYPE_MPEG2_PROGRAM)
        _N_(MEDIASUBTYPE_MPEG2_TRANSPORT)
        _N_(MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE)
        _N_(MEDIASUBTYPE_MPEG2_UDCR_TRANSPORT)
        _N_(MEDIASUBTYPE_MPEG2_VERSIONED_TABLES)
        _N_(MEDIASUBTYPE_MPEG2_VIDEO)
        _N_(MEDIASUBTYPE_MPEG2_WMDRM_TRANSPORT)
        _N_(MEDIASUBTYPE_MPEG2DATA)
        _N_(MEDIASUBTYPE_None)
        _N_(MEDIASUBTYPE_NV12)
        _N_(MEDIASUBTYPE_NV24)
        _N_(MEDIASUBTYPE_Overlay)
        _N_(MEDIASUBTYPE_PCM)
        _N_(MEDIASUBTYPE_PCMAudio_Obsolete)
        _N_(MEDIASUBTYPE_Plum)
        _N_(MEDIASUBTYPE_QTJpeg)
        _N_(MEDIASUBTYPE_QTMovie)
        _N_(MEDIASUBTYPE_QTRle)
        _N_(MEDIASUBTYPE_QTRpza)
        _N_(MEDIASUBTYPE_QTSmc)
        _N_(MEDIASUBTYPE_RAW_SPORT)
        _N_(MEDIASUBTYPE_RGB1)
        _N_(MEDIASUBTYPE_RGB16_D3D_DX7_RT)
        _N_(MEDIASUBTYPE_RGB16_D3D_DX9_RT)
        _N_(MEDIASUBTYPE_RGB24)
        _N_(MEDIASUBTYPE_RGB32)
        _N_(MEDIASUBTYPE_RGB32_D3D_DX7_RT)
        _N_(MEDIASUBTYPE_RGB32_D3D_DX9_RT)
        _N_(MEDIASUBTYPE_RGB4)
        _N_(MEDIASUBTYPE_RGB555)
        _N_(MEDIASUBTYPE_RGB565)
        _N_(MEDIASUBTYPE_RGB8)
        _N_(MEDIASUBTYPE_S340)
        _N_(MEDIASUBTYPE_S342)
        _N_(MEDIASUBTYPE_SDDS)
        _N_(MEDIASUBTYPE_SPDIF_TAG_241h)
        _N_(MEDIASUBTYPE_TELETEXT)
        _N_(MEDIASUBTYPE_TIF_SI)
        _N_(MEDIASUBTYPE_TVMJ)
        _N_(MEDIASUBTYPE_UYVY)
        _N_(MEDIASUBTYPE_VPS)
        _N_(MEDIASUBTYPE_VPVBI)
        _N_(MEDIASUBTYPE_VPVideo)
        _N_(MEDIASUBTYPE_WAKE)
        _N_(MEDIASUBTYPE_WAVE)
        _N_(MEDIASUBTYPE_WSS)
        _N_(MEDIASUBTYPE_Y211)
        _N_(MEDIASUBTYPE_Y411)
        _N_(MEDIASUBTYPE_Y41P)
        _N_(MEDIASUBTYPE_YUY2)
        _N_(MEDIASUBTYPE_YUYV)
        _N_(MEDIASUBTYPE_YV12)
        _N_(MEDIASUBTYPE_YVU9)
        _N_(MEDIASUBTYPE_YVYU)
        _N_(MEDIATYPE_AnalogAudio)
        _N_(MEDIATYPE_AnalogVideo)
        _N_(MEDIATYPE_Audio)
        _N_(MEDIATYPE_AUXLine21Data)
        _N_(MEDIATYPE_DTVCCData)
        _N_(MEDIATYPE_DVD_ENCRYPTED_PACK)
        _N_(MEDIATYPE_DVD_NAVIGATION)
        _N_(MEDIATYPE_File)
        _N_(MEDIATYPE_Interleaved)
        _N_(MEDIATYPE_LMRT)
        _N_(MEDIATYPE_Midi)
        _N_(MEDIATYPE_MPEG1SystemStream)
        _N_(MEDIATYPE_MPEG2_PACK)
        _N_(MEDIATYPE_MPEG2_PES)
        _N_(MEDIATYPE_MPEG2_SECTIONS)
        _N_(MEDIATYPE_MSTVCaption)
        _N_(MEDIATYPE_ScriptCommand)
        _N_(MEDIATYPE_Stream)
        _N_(MEDIATYPE_Text)
        _N_(MEDIATYPE_Timecode)
        _N_(MEDIATYPE_URL_STREAM)
        _N_(MEDIATYPE_VBI)
        _N_(MEDIATYPE_Video)
        _N_(PIN_CATEGORY_ANALOGVIDEOIN)
        _N_(PIN_CATEGORY_CAPTURE)
        _N_(PIN_CATEGORY_CC)
        _N_(PIN_CATEGORY_EDS)
        _N_(PIN_CATEGORY_NABTS)
        _N_(PIN_CATEGORY_PREVIEW)
        _N_(PIN_CATEGORY_STILL)
        _N_(PIN_CATEGORY_TELETEXT)
        _N_(PIN_CATEGORY_TIMECODE)
        _N_(PIN_CATEGORY_VBI)
        _N_(PIN_CATEGORY_VIDEOPORT)
        _N_(PIN_CATEGORY_VIDEOPORT_VBI)
        _N_(PINNAME_BDA_ANALOG_AUDIO)
        _N_(PINNAME_BDA_ANALOG_VIDEO)
        _N_(PINNAME_BDA_FM_RADIO)
        _N_(PINNAME_BDA_IF_PIN)
        _N_(PINNAME_BDA_OPENCABLE_PSIP_PIN)
        _N_(PINNAME_BDA_TRANSPORT)
        _N_(PINNAME_IPSINK_INPUT)
        _N_(PINNAME_MPE)
        _N_(TIME_FORMAT_BYTE)
        _N_(TIME_FORMAT_FIELD)
        _N_(TIME_FORMAT_FRAME)
        _N_(TIME_FORMAT_MEDIA_TIME)
        _N_(TIME_FORMAT_NONE)
        _N_(TIME_FORMAT_SAMPLE)
#undef  _N_
        {0, 0}
    };

    for (const KnownValue* p = knownValues; p->id != 0; ++p) {
        if (*(p->id) == guid) {
            return p->name;
        }
    }

    // Not name found, last chance is default formatting
    return fmt;
}
