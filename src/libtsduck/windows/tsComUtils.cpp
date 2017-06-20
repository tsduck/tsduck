//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Windows Common Object Model (COM) utilities. Windows-specific
//
//-----------------------------------------------------------------------------

#include "tsComUtils.h"
#include "tsComPtr.h"
#include "tsSysUtils.h"
#include "tsStringUtils.h"
#include "tsRegistryUtils.h"
#include "tsFormat.h"
#include "tsFatal.h"


//-----------------------------------------------------------------------------
// Format the message for a COM status
//-----------------------------------------------------------------------------

std::string ts::ComMessage(::HRESULT hr)
{
    // Get DirectShow error message
    char buf[MAX_ERROR_TEXT_LEN];
    ::DWORD size = ::AMGetErrorText(hr, buf, sizeof(buf));
    size = std::max(::DWORD(0), std::min(size, ::DWORD(sizeof(buf) - 1)));
    buf[size] = 0;

    // Remove trailing newlines (if any)
    while (size > 0 && (buf[size-1] == '\n' || buf[size-1] == '\r')) {
        buf[--size] = 0;
    }

    // If DirectShow message is empty, use Win32 error message
    if (size > 0) {
        return buf;
    }
    else {
        return ErrorCodeMessage(hr);
    }
}


//-----------------------------------------------------------------------------
// Check a COM status. In case of error, report an error message.
// Return true is status is success, false if error.
//-----------------------------------------------------------------------------

bool ts::ComSuccess(::HRESULT hr, const std::string& message, ReportInterface& report)
{
    return ComSuccess(hr, message.c_str(), report);
}


bool ts::ComSuccess(::HRESULT hr, const char* message, ReportInterface& report)
{
    if (SUCCEEDED(hr)) {
        return true;
    }

    // Report error message
    if (message != 0) {
        report.error(message + (": " + ComMessage(hr)));
    }
    else {
        report.error("COM error: " + ComMessage(hr));
    }

    return false;
}


//-----------------------------------------------------------------------------
// Convert COM strings to std::string (empty on error)
//-----------------------------------------------------------------------------

std::string ts::ToString(const ::VARIANT& var)
{
    return var.vt == ::VT_BSTR ? ToString(var.bstrVal) : "";
}

std::string ts::ToString(const ::BSTR bstr)
{
    char* cp = _com_util::ConvertBSTRToString(bstr);
    std::string str;
    if (cp != 0) {
        str = cp;
        delete[] cp;
    }
    return str;
}

std::string ts::ToString(const ::WCHAR* str)
{
    // No output buffer => return expected size
    int size = ::WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, ".", NULL);
    if (size <= 0) {
        return "";
    }
    // Now perform actual conversion
    char* buf = new char [size+1];
    int size2 = ::WideCharToMultiByte(CP_ACP, 0, str, -1, buf, size, ".", NULL);
    buf [std::min(size, std::max(0, size2))] = 0;
    std::string result(buf);
    delete[] buf;
    return result;
}


//-----------------------------------------------------------------------------
// Return a string property from the "property bag" of an object
// (defined by an object moniker)
//-----------------------------------------------------------------------------

std::string ts::GetStringPropertyBag(::IMoniker* object_moniker, const ::OLECHAR* property_name, ReportInterface& report)
{
    // Bind to the object's storage, get the "property bag" interface
    ComPtr <::IPropertyBag> pbag;
    ::HRESULT hr = object_moniker->BindToStorage(0,                       // No cached context
                                                 0,                       // Not part of a composite
                                                 ::IID_IPropertyBag,      // ID of requested interface
                                                 (void**)pbag.creator()); // Returned interface
    if (!ComSuccess(hr, "IMoniker::BindToStorage", report)) {
        return "";
    }

    // Get property from property bag

    std::string value;
    ::VARIANT var;
    ::VariantInit(&var);
    hr = pbag->Read(property_name, &var, 0);
    if (ComSuccess(hr, "IPropertyBag::Read", report)) {
        value = ToString(var);
    }
    ::VariantClear(&var);

    return value;
}


//-----------------------------------------------------------------------------
// Format the name of a GUID. Resolve a few known names
//-----------------------------------------------------------------------------

std::string ts::FormatGUID(const ::GUID& guid, bool with_braces)
{
    std::string s(Format("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                         guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                         guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]));
    return with_braces ? "{"+s+"}" : s;
}

std::string ts::NameGUID(const ::GUID& guid)
{
    // Build default formatting
    std::string fmt(FormatGUID(guid, true));
    std::string fmtno(FormatGUID(guid, false));

    // Check registered GUID's
    std::string name;

#define _N_(key,prefix) if (!(name=GetRegistryValue(key+fmt)).empty() || !(name=GetRegistryValue(key+fmtno)).empty()) {return prefix+name;}

    // Windows XP style
    _N_("HKEY_CLASSES_ROOT\\CLSID\\", "CLSID_");
    _N_("HKEY_CLASSES_ROOT\\Interface\\", "IID_");
    _N_("HKEY_CLASSES_ROOT\\DirectShow\\MediaObjects\\", "DirectShow.MediaObject:");
    _N_("HKEY_CLASSES_ROOT\\DirectShow\\MediaObjects\\Categories\\", "DirectShow.MediaObject.Category:");
    _N_("HKEY_CLASSES_ROOT\\Filter\\", "Filter:");

    // Windows 7 style
    _N_("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\", "CLSID_");
    _N_("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Interface\\", "IID_");
    _N_("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\DirectShow\\MediaObjects\\", "DirectShow.MediaObject:");
    _N_("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\DirectShow\\MediaObjects\\Categories\\", "DirectShow.MediaObject.Category:");

#undef _N_

    // Check some predefined GUID values

#define _N_(g) if (g == guid) {return #g;}

    _N_(GUID_NULL);

//  _N_(IID_IBDAComparable);
    _N_(IID_IBDA_NetworkProvider);
    _N_(IID_IBDA_EthernetFilter);
    _N_(IID_IBDA_IPV4Filter);
    _N_(IID_IBDA_IPV6Filter);
    _N_(IID_IBDA_DeviceControl);
    _N_(IID_IBDA_PinControl);
    _N_(IID_IBDA_SignalProperties);
    _N_(IID_IBDA_SignalStatistics);
    _N_(IID_IBDA_Topology);
    _N_(IID_IBDA_VoidTransform);
    _N_(IID_IBDA_NullTransform);
    _N_(IID_IBDA_FrequencyFilter);
    _N_(IID_IBDA_LNBInfo);
    _N_(IID_IBDA_AutoDemodulate);
//  _N_(IID_IBDA_AutoDemodulateEx);
    _N_(IID_IBDA_DigitalDemodulator);
    _N_(IID_IBDA_IPSinkControl);
    _N_(IID_IBDA_IPSinkInfo);
//  _N_(IID_IBDA_EasMessage);
//  _N_(IID_IBDA_TransportStreamInfo);
//  _N_(IID_IBDA_ConditionalAccess);
//  _N_(IID_IBDA_DiagnosticProperties);
//  _N_(IID_IBDA_DRM);
    _N_(IID_IBDA_TIF_REGISTRATION);

    _N_(IID_IAMovieSetup);
    _N_(IID_IAMCopyCaptureFileProgress);
    _N_(IID_IAMStreamControl);
    _N_(IID_IAMStreamConfig);
    _N_(IID_IAMVideoCompression);
    _N_(IID_IAMVfwCaptureDialogs);
    _N_(IID_IAMVfwCompressDialogs);
    _N_(IID_IAMDroppedFrames);
    _N_(IID_IAMAudioInputMixer);
    _N_(IID_IAMBufferNegotiation);
    _N_(IID_IAMAnalogVideoDecoder);
    _N_(IID_IAMVideoProcAmp);
    _N_(IID_IAMCameraControl);
    _N_(IID_IAMVideoControl);
    _N_(IID_IAMCrossbar);
    _N_(IID_IAMTuner);
    _N_(IID_IAMTunerNotification);
    _N_(IID_IAMTVTuner);
    _N_(IID_IAMTVAudio);
    _N_(IID_IAMTVAudioNotification);
    _N_(IID_IAMAnalogVideoEncoder);
    _N_(IID_IAMPhysicalPinInfo);
    _N_(IID_IAMExtDevice);
    _N_(IID_IAMExtTransport);
    _N_(IID_IAMTimecodeReader);
    _N_(IID_IAMTimecodeGenerator);
    _N_(IID_IAMTimecodeDisplay);
    _N_(IID_IAMDevMemoryAllocator);
    _N_(IID_IAMDevMemoryControl);
    _N_(IID_IAMStreamSelect);
    _N_(IID_IAMResourceControl);
    _N_(IID_IAMClockAdjust);
    _N_(IID_IAMFilterMiscFlags);
    _N_(IID_IAMVideoDecimationProperties);
    _N_(IID_IAMLatency);
    _N_(IID_IAMPushSource);
    _N_(IID_IAMDeviceRemoval);
    _N_(IID_IAMAudioRendererStats);
    _N_(IID_IAMGraphStreams);
    _N_(IID_IAMOverlayFX);
    _N_(IID_IAMOpenProgress);
    _N_(IID_IAMClockSlave);
    _N_(IID_IAMGraphBuilderCallback);
    _N_(IID_IAMDecoderCaps);
//  _N_(IID_IAMCertifiedOutputProtection);
    _N_(IID_IAMMultiMediaStream);
    _N_(IID_IAMMediaStream);
    _N_(IID_IAMMediaTypeStream);
    _N_(IID_IAMMediaTypeSample);
    _N_(IID_IAMWMBufferPass);
    _N_(IID_IAMWMBufferPassCallback);
    _N_(IID_IAMTimelineObj);
    _N_(IID_IAMTimelineEffectable);
    _N_(IID_IAMTimelineEffect);
    _N_(IID_IAMTimelineTransable);
    _N_(IID_IAMTimelineSplittable);
    _N_(IID_IAMTimelineTrans);
    _N_(IID_IAMTimelineSrc);
    _N_(IID_IAMTimelineTrack);
    _N_(IID_IAMTimelineVirtualTrack);
    _N_(IID_IAMTimelineComp);
    _N_(IID_IAMTimelineGroup);
    _N_(IID_IAMTimeline);
    _N_(IID_IAMErrorLog);
    _N_(IID_IAMSetErrorLog);
    _N_(IID_IAMVideoAcceleratorNotify);
    _N_(IID_IAMVideoAccelerator);

//  _N_(IID_IKsObject);
    _N_(IID_IKsPin);
//  _N_(IID_IKsPinEx);
//  _N_(IID_IKsPinPipe);
    _N_(IID_IKsDataTypeHandler);
//  _N_(IID_IKsDataTypeCompletion);
    _N_(IID_IKsInterfaceHandler);
//  _N_(IID_IKsClockPropertySet);
//  _N_(IID_IKsAllocator);
//  _N_(IID_IKsAllocatorEx);
    _N_(IID_IKsPropertySet);
    _N_(IID_IKsControl);
//  _N_(IID_IKsAggregateControl);
//  _N_(IID_IKsTopology);
    _N_(CLSID_Proxy);

    _N_(MEDIATYPE_Video);
    _N_(MEDIATYPE_Audio);
    _N_(MEDIATYPE_Text);
    _N_(MEDIATYPE_Midi);
    _N_(MEDIATYPE_Stream);
    _N_(MEDIATYPE_Interleaved);
    _N_(MEDIATYPE_File);
    _N_(MEDIATYPE_ScriptCommand);
    _N_(MEDIATYPE_AUXLine21Data);
//  _N_(MEDIATYPE_DTVCCData);
//  _N_(MEDIATYPE_MSTVCaption);
    _N_(MEDIATYPE_VBI);
    _N_(MEDIATYPE_Timecode);
    _N_(MEDIATYPE_LMRT);
    _N_(MEDIATYPE_URL_STREAM);
    _N_(MEDIATYPE_MPEG1SystemStream);

    _N_(MEDIASUBTYPE_None);
//  _N_(MEDIASUBTYPE_CLPL);
//  _N_(MEDIASUBTYPE_YUYV);
//  _N_(MEDIASUBTYPE_IYUV);
//  _N_(MEDIASUBTYPE_YVU9);
//  _N_(MEDIASUBTYPE_Y411);
//  _N_(MEDIASUBTYPE_Y41P);
//  _N_(MEDIASUBTYPE_YUY2);
//  _N_(MEDIASUBTYPE_YVYU);
//  _N_(MEDIASUBTYPE_UYVY);
//  _N_(MEDIASUBTYPE_Y211);
//  _N_(MEDIASUBTYPE_CLJR);
//  _N_(MEDIASUBTYPE_IF09);
//  _N_(MEDIASUBTYPE_CPLA);
//  _N_(MEDIASUBTYPE_MJPG);
//  _N_(MEDIASUBTYPE_TVMJ);
//  _N_(MEDIASUBTYPE_WAKE);
//  _N_(MEDIASUBTYPE_CFCC);
//  _N_(MEDIASUBTYPE_IJPG);
//  _N_(MEDIASUBTYPE_Plum);
//  _N_(MEDIASUBTYPE_DVCS);
//  _N_(MEDIASUBTYPE_H264);
//  _N_(MEDIASUBTYPE_DVSD);
//  _N_(MEDIASUBTYPE_MDVF);
//  _N_(MEDIASUBTYPE_RGB1);
//  _N_(MEDIASUBTYPE_RGB4);
//  _N_(MEDIASUBTYPE_RGB8);
//  _N_(MEDIASUBTYPE_RGB565);
//  _N_(MEDIASUBTYPE_RGB555);
//  _N_(MEDIASUBTYPE_RGB24);
//  _N_(MEDIASUBTYPE_RGB32);
//  _N_(MEDIASUBTYPE_ARGB1555);
//  _N_(MEDIASUBTYPE_ARGB4444);
//  _N_(MEDIASUBTYPE_ARGB32);
//  _N_(MEDIASUBTYPE_A2R10G10B10);
//  _N_(MEDIASUBTYPE_A2B10G10R10);
//  _N_(MEDIASUBTYPE_AYUV);
//  _N_(MEDIASUBTYPE_AI44);
//  _N_(MEDIASUBTYPE_IA44);
//  _N_(MEDIASUBTYPE_RGB32_D3D_DX7_RT);
//  _N_(MEDIASUBTYPE_RGB16_D3D_DX7_RT);
//  _N_(MEDIASUBTYPE_ARGB32_D3D_DX7_RT);
//  _N_(MEDIASUBTYPE_ARGB4444_D3D_DX7_RT);
//  _N_(MEDIASUBTYPE_ARGB1555_D3D_DX7_RT);
//  _N_(MEDIASUBTYPE_RGB32_D3D_DX9_RT);
//  _N_(MEDIASUBTYPE_RGB16_D3D_DX9_RT);
//  _N_(MEDIASUBTYPE_ARGB32_D3D_DX9_RT);
//  _N_(MEDIASUBTYPE_ARGB4444_D3D_DX9_RT);
//  _N_(MEDIASUBTYPE_ARGB1555_D3D_DX9_RT);
//  _N_(MEDIASUBTYPE_YV12);
//  _N_(MEDIASUBTYPE_NV12);
//  _N_(MEDIASUBTYPE_NV24);
//  _N_(MEDIASUBTYPE_IMC1);
//  _N_(MEDIASUBTYPE_IMC2);
//  _N_(MEDIASUBTYPE_IMC3);
//  _N_(MEDIASUBTYPE_IMC4);
//  _N_(MEDIASUBTYPE_S340);
//  _N_(MEDIASUBTYPE_S342);
    _N_(MEDIASUBTYPE_Overlay);
    _N_(MEDIASUBTYPE_MPEG1Packet);
    _N_(MEDIASUBTYPE_MPEG1Payload);
    _N_(MEDIASUBTYPE_MPEG1AudioPayload);
    _N_(MEDIASUBTYPE_MPEG1System);
    _N_(MEDIASUBTYPE_MPEG1VideoCD);
    _N_(MEDIASUBTYPE_MPEG1Video);
    _N_(MEDIASUBTYPE_MPEG1Audio);
    _N_(MEDIASUBTYPE_Avi);
    _N_(MEDIASUBTYPE_Asf);
//  _N_(MEDIASUBTYPE_QTMovie);
//  _N_(MEDIASUBTYPE_QTRpza);
//  _N_(MEDIASUBTYPE_QTSmc);
//  _N_(MEDIASUBTYPE_QTRle);
//  _N_(MEDIASUBTYPE_QTJpeg);
//  _N_(MEDIASUBTYPE_PCMAudio_Obsolete);
//  _N_(MEDIASUBTYPE_PCM);
//  _N_(MEDIASUBTYPE_WAVE);
//  _N_(MEDIASUBTYPE_AU);
//  _N_(MEDIASUBTYPE_AIFF);
//  _N_(MEDIASUBTYPE_dvsd);
//  _N_(MEDIASUBTYPE_dvhd);
//  _N_(MEDIASUBTYPE_dvsl);
//  _N_(MEDIASUBTYPE_dv25);
//  _N_(MEDIASUBTYPE_dv50);
//  _N_(MEDIASUBTYPE_dvh1);
//  _N_(MEDIASUBTYPE_Line21_BytePair);
//  _N_(MEDIASUBTYPE_Line21_GOPPacket);
//  _N_(MEDIASUBTYPE_Line21_VBIRawData);
//  _N_(MEDIASUBTYPE_708_608Data);
//  _N_(MEDIASUBTYPE_DtvCcData);
    _N_(MEDIASUBTYPE_TELETEXT);
    _N_(MEDIASUBTYPE_WSS);
    _N_(MEDIASUBTYPE_VPS);
    _N_(MEDIASUBTYPE_DRM_Audio);
    _N_(MEDIASUBTYPE_IEEE_FLOAT);
    _N_(MEDIASUBTYPE_DOLBY_AC3_SPDIF);
    _N_(MEDIASUBTYPE_RAW_SPORT);
    _N_(MEDIASUBTYPE_SPDIF_TAG_241h);
    _N_(MEDIASUBTYPE_DssVideo);
    _N_(MEDIASUBTYPE_DssAudio);
    _N_(MEDIASUBTYPE_VPVideo);
    _N_(MEDIASUBTYPE_VPVBI);

    _N_(FORMAT_None);
    _N_(FORMAT_VideoInfo);
    _N_(FORMAT_VideoInfo2);
    _N_(FORMAT_WaveFormatEx);
    _N_(FORMAT_MPEGVideo);
    _N_(FORMAT_MPEGStreams);
    _N_(FORMAT_DvInfo);
//  _N_(FORMAT_525WSS);
    _N_(FORMAT_AnalogVideo);

    _N_(MEDIATYPE_AnalogVideo);
//  _N_(MEDIASUBTYPE_AnalogVideo_NTSC_M);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_B);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_D);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_G);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_H);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_I);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_M);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_N);
//  _N_(MEDIASUBTYPE_AnalogVideo_PAL_N_COMBO);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_B);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_D);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_G);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_H);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_K);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_K1);
//  _N_(MEDIASUBTYPE_AnalogVideo_SECAM_L);
    _N_(MEDIATYPE_AnalogAudio);
    _N_(TIME_FORMAT_NONE);
    _N_(TIME_FORMAT_FRAME);
    _N_(TIME_FORMAT_BYTE);
    _N_(TIME_FORMAT_SAMPLE);
    _N_(TIME_FORMAT_FIELD);
    _N_(TIME_FORMAT_MEDIA_TIME);
    _N_(AMPROPSETID_Pin);
    _N_(PIN_CATEGORY_CAPTURE);
    _N_(PIN_CATEGORY_PREVIEW);
    _N_(PIN_CATEGORY_ANALOGVIDEOIN);
    _N_(PIN_CATEGORY_VBI);
    _N_(PIN_CATEGORY_VIDEOPORT);
    _N_(PIN_CATEGORY_NABTS);
    _N_(PIN_CATEGORY_EDS);
    _N_(PIN_CATEGORY_TELETEXT);
    _N_(PIN_CATEGORY_CC);
    _N_(PIN_CATEGORY_STILL);
    _N_(PIN_CATEGORY_TIMECODE);
    _N_(PIN_CATEGORY_VIDEOPORT_VBI);
    _N_(LOOK_UPSTREAM_ONLY);
    _N_(LOOK_DOWNSTREAM_ONLY);

    _N_(MEDIATYPE_MPEG2_PACK);
    _N_(MEDIATYPE_MPEG2_PES);
//  _N_(MEDIATYPE_MPEG2_SECTIONS);
//  _N_(MEDIASUBTYPE_MPEG2_VERSIONED_TABLES);
//  _N_(MEDIASUBTYPE_ATSC_SI);
//  _N_(MEDIASUBTYPE_DVB_SI);
//  _N_(MEDIASUBTYPE_TIF_SI);
//  _N_(MEDIASUBTYPE_MPEG2DATA);
//  _N_(MEDIASUBTYPE_MPEG2_WMDRM_TRANSPORT);
    _N_(MEDIASUBTYPE_MPEG2_VIDEO);
    _N_(FORMAT_MPEG2_VIDEO);
    _N_(FORMAT_VIDEOINFO2);
    _N_(MEDIASUBTYPE_MPEG2_PROGRAM);
    _N_(MEDIASUBTYPE_MPEG2_TRANSPORT);
    _N_(MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE);
//  _N_(MEDIASUBTYPE_MPEG2_UDCR_TRANSPORT);
    _N_(MEDIASUBTYPE_MPEG2_AUDIO);
    _N_(MEDIASUBTYPE_DOLBY_AC3);
    _N_(MEDIASUBTYPE_DVD_SUBPICTURE);
    _N_(MEDIASUBTYPE_DVD_LPCM_AUDIO);
    _N_(MEDIASUBTYPE_DTS);
    _N_(MEDIASUBTYPE_SDDS);
    _N_(MEDIATYPE_DVD_ENCRYPTED_PACK);
    _N_(MEDIATYPE_DVD_NAVIGATION);
    _N_(MEDIASUBTYPE_DVD_NAVIGATION_PCI);
    _N_(MEDIASUBTYPE_DVD_NAVIGATION_DSI);
    _N_(MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER);
    _N_(FORMAT_MPEG2Video);
    _N_(FORMAT_DolbyAC3);
    _N_(FORMAT_MPEG2Audio);
    _N_(FORMAT_DVD_LPCMAudio);
//  _N_(AM_KSPROPSETID_AC3);
//  _N_(AM_KSPROPSETID_DvdSubPic);
//  _N_(AM_KSPROPSETID_CopyProt);
//  _N_(AM_KSPROPSETID_TSRateChange);
//  _N_(AM_KSPROPSETID_DVD_RateChange);
//  _N_(AM_KSPROPSETID_DvdKaraoke);
//  _N_(AM_KSPROPSETID_FrameStep);
    _N_(AM_KSCATEGORY_CAPTURE);
    _N_(AM_KSCATEGORY_RENDER);
    _N_(AM_KSCATEGORY_DATACOMPRESSOR);
    _N_(AM_KSCATEGORY_AUDIO);
    _N_(AM_KSCATEGORY_VIDEO);
    _N_(AM_KSCATEGORY_TVTUNER);
    _N_(AM_KSCATEGORY_CROSSBAR);
    _N_(AM_KSCATEGORY_TVAUDIO);
    _N_(AM_KSCATEGORY_VBICODEC);
    _N_(AM_KSCATEGORY_SPLITTER);
//  _N_(AM_KSCATEGORY_VBICODEC_MI);
    _N_(IID_IKsInterfaceHandler);
    _N_(IID_IKsDataTypeHandler);
    _N_(IID_IKsPin);
    _N_(IID_IKsControl);
    _N_(IID_IKsPinFactory);
    _N_(AM_INTERFACESETID_Standard);

    _N_(EVENTID_TuningChanging);
    _N_(EVENTID_TuningChanged);
    _N_(EVENTID_CADenialCountChanged);
    _N_(EVENTID_SignalStatusChanged);
    _N_(EVENTID_NewSignalAcquired);
    _N_(EVENTID_EASMessageReceived);
    _N_(EVENTID_PSITable);
    _N_(EVENTID_CardStatusChanged);
    _N_(EVENTID_DRMParingStatusChanged);
    _N_(EVENTID_MMIMessage);
    _N_(EVENTID_EntitlementChanged);
    _N_(EVENTID_STBChannelNumber);

    _N_(KSDATAFORMAT_TYPE_BDA_ANTENNA);
    _N_(KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT);
    _N_(KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT);
    _N_(KSDATAFORMAT_TYPE_BDA_IF_SIGNAL);
    _N_(KSDATAFORMAT_TYPE_MPEG2_SECTIONS);
    _N_(KSDATAFORMAT_SUBTYPE_ATSC_SI);
    _N_(KSDATAFORMAT_SUBTYPE_DVB_SI);
    _N_(KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP);
    _N_(KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_OOB_PSIP);
    _N_(PINNAME_BDA_TRANSPORT);
    _N_(PINNAME_BDA_ANALOG_VIDEO);
    _N_(PINNAME_BDA_ANALOG_AUDIO);
    _N_(PINNAME_BDA_FM_RADIO);
    _N_(PINNAME_BDA_IF_PIN);
    _N_(PINNAME_BDA_OPENCABLE_PSIP_PIN);
//  _N_(KSPROPSETID_BdaEthernetFilter);
//  _N_(KSPROPSETID_BdaIPv4Filter);
//  _N_(KSPROPSETID_BdaIPv6Filter);
//  _N_(KSPROPSETID_BdaSignalStats);
//  _N_(KSMETHODSETID_BdaChangeSync);
//  _N_(KSMETHODSETID_BdaDeviceConfiguration);
//  _N_(KSPROPSETID_BdaTopology);
//  _N_(KSPROPSETID_BdaPinControl);
//  _N_(KSEVENTSETID_BdaPinEvent);
//  _N_(KSPROPSETID_BdaVoidTransform);
//  _N_(KSPROPSETID_BdaNullTransform);
//  _N_(KSPROPSETID_BdaFrequencyFilter);
//  _N_(KSEVENTSETID_BdaTunerEvent);
//  _N_(KSPROPSETID_BdaLNBInfo);
//  _N_(KSPROPSETID_BdaDigitalDemodulator);
//  _N_(KSPROPSETID_BdaAutodemodulate);
//  _N_(KSPROPSETID_BdaTableSection);
//  _N_(KSPROPSETID_BdaPIDFilter);
//  _N_(KSPROPSETID_BdaCA);
//  _N_(KSEVENTSETID_BdaCAEvent);
    _N_(KSCATEGORY_BDA_RECEIVER_COMPONENT);
    _N_(KSCATEGORY_BDA_NETWORK_TUNER);
    _N_(KSCATEGORY_BDA_NETWORK_EPG);
    _N_(KSCATEGORY_IP_SINK);
    _N_(KSCATEGORY_BDA_NETWORK_PROVIDER);
    _N_(KSCATEGORY_BDA_TRANSPORT_INFORMATION);
    _N_(KSNODE_BDA_RF_TUNER);
    _N_(KSNODE_BDA_ANALOG_DEMODULATOR);
    _N_(KSNODE_BDA_QAM_DEMODULATOR);
    _N_(KSNODE_BDA_QPSK_DEMODULATOR);
    _N_(KSNODE_BDA_8VSB_DEMODULATOR);
    _N_(KSNODE_BDA_COFDM_DEMODULATOR);
    _N_(KSNODE_BDA_8PSK_DEMODULATOR);
    _N_(KSNODE_BDA_OPENCABLE_POD);
    _N_(KSNODE_BDA_COMMON_CA_POD);
    _N_(KSNODE_BDA_PID_FILTER);
    _N_(KSNODE_IP_SINK);
    _N_(KSNODE_BDA_VIDEO_ENCODER);
    _N_(PINNAME_IPSINK_INPUT);
    _N_(KSDATAFORMAT_TYPE_BDA_IP);
    _N_(KSDATAFORMAT_SUBTYPE_BDA_IP);
    _N_(KSDATAFORMAT_SPECIFIER_BDA_IP);
    _N_(KSDATAFORMAT_TYPE_BDA_IP_CONTROL);
    _N_(KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL);
    _N_(PINNAME_MPE);
    _N_(KSDATAFORMAT_TYPE_MPE);
    _N_(DIGITAL_CABLE_NETWORK_TYPE);
    _N_(ANALOG_TV_NETWORK_TYPE);
    _N_(ANALOG_AUXIN_NETWORK_TYPE);
    _N_(ANALOG_FM_NETWORK_TYPE);
    _N_(ISDB_TERRESTRIAL_TV_NETWORK_TYPE);
    _N_(ISDB_SATELLITE_TV_NETWORK_TYPE);
    _N_(ISDB_CABLE_TV_NETWORK_TYPE);
    _N_(DIRECT_TV_SATELLITE_TV_NETWORK_TYPE);
    _N_(ATSC_TERRESTRIAL_TV_NETWORK_TYPE);
    _N_(DVB_TERRESTRIAL_TV_NETWORK_TYPE);
    _N_(DVB_SATELLITE_TV_NETWORK_TYPE);
    _N_(DVB_CABLE_TV_NETWORK_TYPE);

    // From tsSysUtils.windows.h
    _N_(CLSID_SinkFilter);

#undef _N_

    // Not name found, last chance is default formatting
    return fmt;
}
