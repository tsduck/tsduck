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
//  DirectShow & BDA utilities, Windows-specific.
//
//-----------------------------------------------------------------------------

#include "tsDirectShowUtils.h"
#include "tsMediaTypeUtils.h"
#include "tsComUtils.h"
#include "tsMemoryUtils.h"
#include "tsNullReport.h"
#include "tsFormat.h"
#include "tsDecimal.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Get list of pins on a filter (use flags from enum above)
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::GetPin(PinPtrVector& result, ::IBaseFilter* filter, int flags, ReportInterface& report)
{
    // Clear result vector (explicitely nullify previous values to release objects)
    ComVectorClear(result);

    // If neither input nor output, neither connected nor unconnected, nothing to search.
    if ((flags & (xPIN_INPUT | xPIN_OUTPUT)) == 0 || (flags & (xPIN_CONNECTED | xPIN_UNCONNECTED)) == 0) {
        return true;
    }

    // Create a pin enumerator
    ComPtr <::IEnumPins> enum_pins;
    ::HRESULT hr = filter->EnumPins(enum_pins.creator());
    if (!ComSuccess(hr, "IBaseFilter::EnumPins", report)) {
        return false;
    }

    // Loop on all pins
    ComPtr <::IPin> pin;
    while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {
        // Query direction of this pin
        ::PIN_DIRECTION dir;
        if (FAILED(pin->QueryDirection(&dir)) ||
            ((dir != ::PINDIR_INPUT || (flags & xPIN_INPUT) == 0) &&
             (dir != ::PINDIR_OUTPUT || (flags & xPIN_OUTPUT) == 0))) {
            continue; // not the right direction, see next pin
        }
        // Query connected pin
        ComPtr<::IPin> partner;
        bool connected = SUCCEEDED(pin->ConnectedTo(partner.creator()));
        if ((connected && (flags & xPIN_CONNECTED) != 0) || (!connected && (flags & xPIN_UNCONNECTED) != 0)) {
            // Keep this pin
            result.push_back(pin);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Directly connect two filters using whatever output and input pin.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::ConnectFilters(::IGraphBuilder* graph,
                        ::IBaseFilter* filter1,
                        ::IBaseFilter* filter2,
                        ReportInterface& report)
{
    // Get unconnected pins
    PinPtrVector pins1;
    PinPtrVector pins2;
    if (!GetPin(pins1, filter1, xPIN_OUTPUT | xPIN_UNCONNECTED, report) ||
        !GetPin(pins2, filter2, xPIN_INPUT | xPIN_UNCONNECTED, report)) {
        return false;
    }

    // Try all combinations
    for (size_t i1 = 0; i1 < pins1.size(); ++i1) {
        for (size_t i2 = 0; i2 < pins2.size(); ++i2) {
            ::HRESULT hr = graph->Connect(pins1[i1].pointer(), pins2[i2].pointer());
            if (SUCCEEDED(hr)) {
                return true;
            }
            report.debug(Format("failed to connect pins, status = 0x%08X, ", int(hr)) + ComMessage(hr));
        }
    }

    // No connection made
    return false;
}


//-----------------------------------------------------------------------------
// In a DirectShow filter graph, cleanup everything downstream a filter.
//-----------------------------------------------------------------------------

bool ts::CleanupDownstream(::IGraphBuilder* graph, ::IBaseFilter* filter, ReportInterface& report)
{
    // Get connected output pins
    PinPtrVector pins;
    if (!GetPin(pins, filter, xPIN_OUTPUT | xPIN_CONNECTED, report)) {
        return false;
    }

    // Final status
    bool ok = true;

    // Loop on all connected output pins
    for (size_t pin_index = 0; pin_index < pins.size(); ++pin_index) {

        const ComPtr<::IPin>& pin(pins[pin_index]);
        ComPtr<::IPin> next_pin;
        ComPtr<::IBaseFilter> next_filter;

        // Get connected pin (input pin of next filter)
        ::HRESULT hr = pin->ConnectedTo(next_pin.creator());
        ok = ComSuccess(hr, "IPin::ConnectedTo", report) && ok;

        // Get next filter
        if (!next_pin.isNull()) {
            ::PIN_INFO pin_info;
            TS_ZERO(pin_info);
            hr = next_pin->QueryPinInfo(&pin_info);
            ok = ComSuccess(hr, "IPin::QueryPinInfo", report) && ok;
            next_filter = pin_info.pFilter; // pointer becomes managed if not null
        }

        // Recurse to cleanup downstream next filter
        if (!next_filter.isNull()) {
            ok = CleanupDownstream(graph, next_filter.pointer(), report) && ok;
        }

        // Disconnect pin to next filter
        hr = pin->Disconnect();
        ok = ComSuccess(hr, "IPin::Disconnect", report) && ok;

        // Remove next filter from the graph
        if (!next_filter.isNull()) {
            hr = graph->RemoveFilter(next_filter.pointer());
            ok = ComSuccess(hr, "IFilterGraph::RemoveFilter", report) && ok;
        }
    }

    return ok;
}


//-----------------------------------------------------------------------------
// Show selected properties of a COM object
//-----------------------------------------------------------------------------

namespace {
    template <class COMCLASS>
    void DisplayObject(std::ostream& strm, const std::string& margin, const ts::ComPtr<COMCLASS>& obj, ts::ReportInterface& report);
}


//-----------------------------------------------------------------------------
// List some known interfaces that an object may expose
//-----------------------------------------------------------------------------

namespace {
    template <class COMCLASS>
    void DisplayInterfaces(std::ostream& strm, const std::string& margin, const ts::ComPtr<COMCLASS>& obj, ts::ReportInterface& report)
    {
#define _I_(p,id) if (obj.expose(p##id)) {strm << margin << "interface " #id << std::endl;}
        _I_(IID_, IAMAnalogVideoDecoder);
        _I_(IID_, IAMAnalogVideoEncoder);
        _I_(IID_, IAMAudioInputMixer);
        _I_(IID_, IAMAudioRendererStats);
        _I_(IID_, IAMBufferNegotiation);
        _I_(IID_, IAMCameraControl);
//      _I_(IID_, IAMCertifiedOutputProtection);
        _I_(IID_, IAMClockAdjust);
        _I_(IID_, IAMClockSlave);
        _I_(IID_, IAMCopyCaptureFileProgress);
        _I_(IID_, IAMCrossbar);
        _I_(IID_, IAMDecoderCaps);
        _I_(IID_, IAMDevMemoryAllocator);
        _I_(IID_, IAMDevMemoryControl);
        _I_(IID_, IAMDeviceRemoval);
        _I_(IID_, IAMDroppedFrames);
        _I_(IID_, IAMErrorLog);
        _I_(IID_, IAMExtDevice);
        _I_(IID_, IAMExtTransport);
        _I_(IID_, IAMFilterGraphCallback);
        _I_(IID_, IAMFilterMiscFlags);
        _I_(IID_, IAMGraphBuilderCallback);
        _I_(IID_, IAMGraphStreams);
        _I_(IID_, IAMLatency);
        _I_(IID_, IAMMediaStream);
        _I_(IID_, IAMMediaTypeSample);
        _I_(IID_, IAMMediaTypeStream);
        _I_(IID_, IAMMultiMediaStream);
        _I_(IID_, IAMOpenProgress);
        _I_(IID_, IAMOverlayFX);
        _I_(IID_, IAMPhysicalPinInfo);
        _I_(IID_, IAMPushSource);
        _I_(IID_, IAMResourceControl);
        _I_(IID_, IAMSetErrorLog);
        _I_(IID_, IAMStreamConfig);
        _I_(IID_, IAMStreamControl);
        _I_(IID_, IAMStreamSelect);
        _I_(IID_, IAMTVAudio);
        _I_(IID_, IAMTVAudioNotification);
        _I_(IID_, IAMTVTuner);
        _I_(IID_, IAMTimecodeDisplay);
        _I_(IID_, IAMTimecodeGenerator);
        _I_(IID_, IAMTimecodeReader);
        _I_(IID_, IAMTimeline);
        _I_(IID_, IAMTimelineComp);
        _I_(IID_, IAMTimelineEffect);
        _I_(IID_, IAMTimelineEffectable);
        _I_(IID_, IAMTimelineGroup);
        _I_(IID_, IAMTimelineObj);
        _I_(IID_, IAMTimelineSplittable);
        _I_(IID_, IAMTimelineSrc);
        _I_(IID_, IAMTimelineTrack);
        _I_(IID_, IAMTimelineTrans);
        _I_(IID_, IAMTimelineTransable);
        _I_(IID_, IAMTimelineVirtualTrack);
        _I_(IID_, IAMTuner);
        _I_(IID_, IAMTunerNotification);
        _I_(IID_, IAMVfwCaptureDialogs);
        _I_(IID_, IAMVfwCompressDialogs);
        _I_(IID_, IAMVideoAccelerator);
        _I_(IID_, IAMVideoAcceleratorNotify);
        _I_(IID_, IAMVideoCompression);
        _I_(IID_, IAMVideoControl);
        _I_(IID_, IAMVideoDecimationProperties);
        _I_(IID_, IAMVideoProcAmp);
        _I_(IID_, IAMWMBufferPass);
        _I_(IID_, IAMWMBufferPassCallback);
        _I_(IID_, IAMovieSetup);
        _I_(IID_, IAsyncReader);
//      _I_(IID_, IAttributeGet);
//      _I_(IID_, IAttributeSet);
//      _I_(IID_, IBDAComparable);
        _I_(IID_, IBDA_AutoDemodulate);
//      _I_(IID_, IBDA_AutoDemodulateEx);
//      _I_(IID_, IBDA_ConditionalAccess);
//      _I_(IID_, IBDA_DRM);
        _I_(IID_, IBDA_DeviceControl);
//      _I_(IID_, IBDA_DiagnosticProperties);
        _I_(IID_, IBDA_DigitalDemodulator);
        _I_(IID_, IBDA_DigitalDemodulator2);
        _I_(IID_, IBDA_DigitalDemodulator3);
        _I_(IID_, IBDA_DiseqCommand);
//      _I_(IID_, IBDA_EasMessage);
        _I_(IID_, IBDA_EthernetFilter);
        _I_(IID_, IBDA_FrequencyFilter);
        _I_(IID_, IBDA_IPSinkControl);
        _I_(IID_, IBDA_IPSinkInfo);
        _I_(IID_, IBDA_IPV4Filter);
        _I_(IID_, IBDA_IPV6Filter);
        _I_(IID_, IBDA_LNBInfo);
        _I_(IID_, IBDA_NetworkProvider);
        _I_(IID_, IBDA_NullTransform);
        _I_(IID_, IBDA_PinControl);
        _I_(IID_, IBDA_SignalProperties);
        _I_(IID_, IBDA_SignalStatistics);
        _I_(IID_, IBDA_TIF_REGISTRATION);
        _I_(IID_, IBDA_Topology);
//      _I_(IID_, IBDA_TransportStreamInfo);
        _I_(IID_, IBDA_VoidTransform);
        _I_(IID_, IBPCSatelliteTuner);
        _I_(IID_, IBaseFilter);
        _I_(IID_, ICaptureGraphBuilder);
        _I_(IID_, ICaptureGraphBuilder2);
        _I_(IID_, ICodecAPI);
        _I_(IID_, IConfigAviMux);
        _I_(IID_, IConfigInterleaving);
        _I_(IID_, ICreateDevEnum);
        _I_(IID_, IDDrawExclModeVideo);
        _I_(IID_, IDDrawExclModeVideoCallback);
        _I_(IID_, IDVEnc);
        _I_(IID_, IDVRGB219);
        _I_(IID_, IDVSplitter);
        _I_(IID_, IDecimateVideoImage);
        _I_(IID_, IDistributorNotify);
        _I_(IID_, IDrawVideoImage);
        _I_(IID_, IDVBCLocator);
        _I_(IID_, IDVBSLocator);
        _I_(IID_, IDVBSTuningSpace);
        _I_(IID_, IDVBTLocator);
        _I_(IID_, IDVBTuneRequest);
        _I_(IID_, IDVBTuningSpace);
        _I_(IID_, IDVBTuningSpace2);
        _I_(IID_, IDvbCableDeliverySystemDescriptor);
        _I_(IID_, IDvbFrequencyListDescriptor);
        _I_(IID_, IDvbLogicalChannelDescriptor);
        _I_(IID_, IDvbSatelliteDeliverySystemDescriptor);
        _I_(IID_, IDvbServiceDescriptor);
        _I_(IID_, IDvbSiParser);
        _I_(IID_, IDvbTerrestrialDeliverySystemDescriptor);
        _I_(IID_, IDvdCmd);
        _I_(IID_, IDvdControl);
        _I_(IID_, IDvdControl2);
        _I_(IID_, IDvdGraphBuilder);
        _I_(IID_, IDvdInfo);
        _I_(IID_, IDvdInfo2);
        _I_(IID_, IDvdState);
        _I_(IID_, IEncoderAPI);
        _I_(IID_, IEnumFilters);
        _I_(IID_, IEnumMediaTypes);
        _I_(IID_, IEnumPins);
        _I_(IID_, IEnumRegFilters);
        _I_(IID_, IEnumStreamIdMap);
        _I_(IID_, IEnumTuneRequests);
        _I_(IID_, IEnumTuningSpaces);
        _I_(IID_, IFileSinkFilter);
        _I_(IID_, IFileSinkFilter2);
        _I_(IID_, IFileSourceFilter);
        _I_(IID_, IFilterChain);
        _I_(IID_, IFilterGraph);
        _I_(IID_, IFilterGraph2);
//      _I_(IID_, IFilterGraph3);
        _I_(IID_, IFilterMapper);
        _I_(IID_, IFilterMapper2);
        _I_(IID_, IFilterMapper3);
        _I_(IID_, IFrequencyMap);
        _I_(IID_, IGetCapabilitiesKey);
        _I_(IID_, IGraphBuilder);
        _I_(IID_, IGraphConfig);
        _I_(IID_, IGraphConfigCallback);
        _I_(IID_, IGraphVersion);
        _I_(IID_, IIPDVDec);
        _I_(IID_, IKsControl);
        _I_(IID_, IKsDataTypeHandler);
        _I_(IID_, IKsInterfaceHandler);
        _I_(IID_, IKsPin);
        _I_(IID_, IKsPropertySet);
        _I_(IID_, IKsTopologyInfo);
        _I_(IID_, IMPEG2Component);
        _I_(IID_, IMPEG2ComponentType);
        _I_(IID_, IMPEG2PIDMap);
        _I_(IID_, IMPEG2StreamIdMap);
        _I_(IID_, IMPEG2TuneRequest);
        _I_(IID_, IMPEG2TuneRequestFactory);
        _I_(IID_, IMPEG2TuneRequestSupport);
        _I_(IID_, IMPEG2_TIF_CONTROL);
        _I_(IID_, IMediaEventSink);
        _I_(IID_, IMediaFilter);
        _I_(IID_, IMediaPropertyBag);
        _I_(IID_, IMediaSample);
        _I_(IID_, IMediaSample2);
//      _I_(IID_, IMediaSample2Config);
        _I_(IID_, IMediaSeeking);
        _I_(IID_, IMemAllocator);
        _I_(IID_, IMemAllocatorCallbackTemp);
        _I_(IID_, IMemAllocatorNotifyCallbackTemp);
        _I_(IID_, IMemInputPin);
        _I_(IID_, IMpeg2Data);
        _I_(IID_, IMpeg2Demultiplexer);
//      _I_(IID_, IMpeg2Stream);
//      _I_(IID_, IMpeg2TableFilter);
        _I_(IID_, IOverlay);
        _I_(IID_, IOverlayNotify);
        _I_(IID_, IOverlayNotify2);
        _I_(IID_, IPersistMediaPropertyBag);
        _I_(IID_, IPin);
        _I_(IID_, IPinConnection);
        _I_(IID_, IPinFlowControl);
        _I_(IID_, IQualityControl);
        _I_(IID_, IReferenceClock);
        _I_(IID_, IReferenceClock2);
//      _I_(IID_, IReferenceClockTimerControl);
        _I_(IID_, IRegisterServiceProvider);
//      _I_(IID_, IRegisterTuner);
        _I_(IID_, IResourceConsumer);
        _I_(IID_, IResourceManager);
        _I_(IID_, IScanningTuner);
//      _I_(IID_, IScanningTunerEx);
        _I_(IID_, ISeekingPassThru);
        _I_(IID_, ISelector);
        _I_(IID_, IStreamBuilder);
        _I_(IID_, ITuneRequest);
        _I_(IID_, ITuneRequestInfo);
        _I_(IID_, ITuner);
//      _I_(IID_, ITunerCap);
        _I_(IID_, ITuningSpace);
        _I_(IID_, ITuningSpaceContainer);
        _I_(IID_, ITuningSpaces);
        _I_(IID_, IVMRAspectRatioControl);
        _I_(IID_, IVMRDeinterlaceControl);
        _I_(IID_, IVMRFilterConfig);
        _I_(IID_, IVMRImageCompositor);
        _I_(IID_, IVMRImagePresenter);
        _I_(IID_, IVMRImagePresenterConfig);
        _I_(IID_, IVMRImagePresenterExclModeConfig);
        _I_(IID_, IVMRMixerBitmap);
        _I_(IID_, IVMRMixerControl);
        _I_(IID_, IVMRMonitorConfig);
        _I_(IID_, IVMRSurface);
        _I_(IID_, IVMRSurfaceAllocator);
        _I_(IID_, IVMRSurfaceAllocatorNotify);
        _I_(IID_, IVMRVideoStreamControl);
        _I_(IID_, IVMRWindowlessControl);
        _I_(IID_, IVPManager);
        _I_(IID_, IVideoEncoder);
        _I_(IID_, IVideoFrameStep);
#undef _I_
    }
}


//-----------------------------------------------------------------------------
// Show properties support through IKsPropertySet for a COM object
//-----------------------------------------------------------------------------

namespace {
    void DisplayIKsPropertySet(std::ostream& strm, const std::string& margin, ::IUnknown* object, ts::ReportInterface& report)
    {
        using namespace ts;

        // Check if the filter supports IKsPropertySet
        ComPtr <::IKsPropertySet> propset;
        propset.queryInterface(object, IID_IKsPropertySet, NULLREP);
        if (propset.isNull()) {
            return;
        }
        strm << margin << "IKsPropertySet properties support:" << std::endl;

        // Check some supported properties
        ::DWORD support;

#define _P_(ps,id) \
        if (SUCCEEDED(propset->QuerySupported(KSPROPSETID_Bda##ps, KSPROPERTY_BDA_##id, &support)) && support != 0) {           \
            strm << margin << "  " #id " (" #ps ") :";  \
            if (support & KSPROPERTY_SUPPORT_GET) {     \
                strm << " get";                         \
            }                                           \
            if (support & KSPROPERTY_SUPPORT_SET) {     \
                strm << " set";                         \
            }                                           \
            strm << std::endl;                          \
        }

        _P_(SignalStats, SIGNAL_STRENGTH);
        _P_(SignalStats, SIGNAL_QUALITY);
        _P_(SignalStats, SIGNAL_PRESENT);
        _P_(SignalStats, SIGNAL_LOCKED);
        _P_(SignalStats, SAMPLE_TIME);
        _P_(SignalStats, SIGNAL_LOCK_CAPS);
        _P_(SignalStats, SIGNAL_LOCK_TYPE);

        _P_(FrequencyFilter, RF_TUNER_FREQUENCY);
        _P_(FrequencyFilter, RF_TUNER_POLARITY);
        _P_(FrequencyFilter, RF_TUNER_RANGE);
        _P_(FrequencyFilter, RF_TUNER_TRANSPONDER);
        _P_(FrequencyFilter, RF_TUNER_BANDWIDTH);
        _P_(FrequencyFilter, RF_TUNER_FREQUENCY_MULTIPLIER);
        _P_(FrequencyFilter, RF_TUNER_CAPS);
        _P_(FrequencyFilter, RF_TUNER_SCAN_STATUS);
        _P_(FrequencyFilter, RF_TUNER_STANDARD);
        _P_(FrequencyFilter,    RF_TUNER_STANDARD_MODE);

        _P_(DigitalDemodulator, MODULATION_TYPE);
        _P_(DigitalDemodulator, INNER_FEC_TYPE);
        _P_(DigitalDemodulator, INNER_FEC_RATE);
        _P_(DigitalDemodulator, OUTER_FEC_TYPE);
        _P_(DigitalDemodulator, OUTER_FEC_RATE);
        _P_(DigitalDemodulator, SYMBOL_RATE);
        _P_(DigitalDemodulator, SPECTRAL_INVERSION);
        _P_(DigitalDemodulator, GUARD_INTERVAL);
        _P_(DigitalDemodulator, TRANSMISSION_MODE);
        _P_(DigitalDemodulator, ROLL_OFF);
        _P_(DigitalDemodulator, PILOT);

        _P_(LNBInfo, LNB_LOF_LOW_BAND);
        _P_(LNBInfo, LNB_LOF_HIGH_BAND);
        _P_(LNBInfo, LNB_SWITCH_FREQUENCY);

#undef _P_
    }
}

//-----------------------------------------------------------------------------
// Show properties support through IKsControl for a COM object
//-----------------------------------------------------------------------------

namespace {
    void DisplayIKsControl(std::ostream& strm, const std::string& margin, ::IUnknown* object, ts::ReportInterface& report)
    {
        using namespace ts;

        // Check if the filter supports IKsPropertySet
        ComPtr<::IKsControl> control;
        control.queryInterface(object, IID_IKsControl, NULLREP);
        if (control.isNull()) {
            return;
        }
        strm << margin << "IKsControl properties support:" << std::endl;

        // Check some supported properties
        ::KSPROPERTY prop;
        ::DWORD support;
        ::ULONG retsize;
        ::HRESULT hr;

#define _P_(ps,id)                                                      \
        prop.Set = KSPROPSETID_Bda##ps;                                 \
        prop.Id = KSPROPERTY_BDA_##id;                                  \
        prop.Flags = KSPROPERTY_TYPE_BASICSUPPORT;                      \
        support = 0;                                                    \
        hr = control->KsProperty(&prop, sizeof(prop), &support, sizeof(support), &retsize); \
        if(SUCCEEDED(hr) && support != 0) {                             \
            strm << margin << "  " #id " (" #ps ") :";                  \
            if (support & KSPROPERTY_TYPE_GET) {                        \
                strm << " get";                                         \
            }                                                           \
            if (support & KSPROPERTY_TYPE_SET) {                        \
                strm << " set";                                         \
            }                                                           \
            strm << std::endl;                                          \
        }

        _P_(SignalStats, SIGNAL_STRENGTH);
        _P_(SignalStats, SIGNAL_QUALITY);
        _P_(SignalStats, SIGNAL_PRESENT);
        _P_(SignalStats, SIGNAL_LOCKED);
        _P_(SignalStats, SAMPLE_TIME);
        _P_(SignalStats, SIGNAL_LOCK_CAPS);
        _P_(SignalStats, SIGNAL_LOCK_TYPE);

        _P_(FrequencyFilter, RF_TUNER_FREQUENCY);
        _P_(FrequencyFilter, RF_TUNER_POLARITY);
        _P_(FrequencyFilter, RF_TUNER_RANGE);
        _P_(FrequencyFilter, RF_TUNER_TRANSPONDER);
        _P_(FrequencyFilter, RF_TUNER_BANDWIDTH);
        _P_(FrequencyFilter, RF_TUNER_FREQUENCY_MULTIPLIER);
        _P_(FrequencyFilter, RF_TUNER_CAPS);
        _P_(FrequencyFilter, RF_TUNER_SCAN_STATUS);
        _P_(FrequencyFilter, RF_TUNER_STANDARD);
        _P_(FrequencyFilter, RF_TUNER_STANDARD_MODE);

        _P_(DigitalDemodulator, MODULATION_TYPE);
        _P_(DigitalDemodulator, INNER_FEC_TYPE);
        _P_(DigitalDemodulator, INNER_FEC_RATE);
        _P_(DigitalDemodulator, OUTER_FEC_TYPE);
        _P_(DigitalDemodulator, OUTER_FEC_RATE);
        _P_(DigitalDemodulator, SYMBOL_RATE);
        _P_(DigitalDemodulator, SPECTRAL_INVERSION);
        _P_(DigitalDemodulator, GUARD_INTERVAL);
        _P_(DigitalDemodulator, TRANSMISSION_MODE);
        _P_(DigitalDemodulator, ROLL_OFF);
        _P_(DigitalDemodulator, PILOT);

        _P_(LNBInfo, LNB_LOF_LOW_BAND);
        _P_(LNBInfo, LNB_LOF_HIGH_BAND);
        _P_(LNBInfo, LNB_SWITCH_FREQUENCY);

#undef _P_
    }
}


//-----------------------------------------------------------------------------
// Show IKsTopologyInfo for a COM object
//-----------------------------------------------------------------------------

namespace {
    void DisplayIKsTopologyInfo(std::ostream& strm, const std::string& margin, ::IUnknown* object, ts::ReportInterface& report)
    {
        using namespace ts;

        // Check if the filter supports IKsTopologyInfo
        ComPtr <::IKsTopologyInfo> topinfo;
        topinfo.queryInterface(object, IID_IKsTopologyInfo, NULLREP);
        if (topinfo.isNull()) {
            return;
        }
        strm << margin << "IKsTopologyInfo:" << std::endl;

        // List categories
        ::DWORD cat_count;
        ::HRESULT hr = topinfo->get_NumCategories(&cat_count);
        if (ComSuccess(hr, "IKsTopologyInfo::get_NumCategories", report)) {
            strm << margin << "  Categories:";
            if (cat_count <= 0) {
                strm << " none";
            }
            for (::DWORD cat = 0; cat < cat_count; cat++) {
                ::GUID category;
                hr = topinfo->get_Category(cat, &category);
                if (ComSuccess(hr, "IKsTopologyInfo::get_Category", report)) {
                    strm << " " << NameGUID(category);
                }
            }
            strm << std::endl;
        }

        // List nodes
        ::DWORD node_count;
        hr = topinfo->get_NumNodes(&node_count);
        if (ComSuccess(hr, "IKsTopologyInfo::get_NumNodes", report)) {
            if (node_count <= 0) {
                strm << margin << "  No node found" << std::endl;
            }
            for (::DWORD n = 0; n < node_count; n++) {
                strm << margin << "  Node " << n;
                ::GUID node_type;
                hr = topinfo->get_NodeType(n, &node_type);
                if (ComSuccess(hr, "IKsTopologyInfo::get_NodeType", report)) {
                    strm << ", type " << NameGUID(node_type);
                }
                static const ::DWORD MAX_NODE_NAME = 256;
                ::WCHAR name [MAX_NODE_NAME];
                ::DWORD name_size;
                hr = topinfo->get_NodeName(n, name, MAX_NODE_NAME, &name_size);
                if (ComSuccess(hr, "IKsTopologyInfo::get_NodeName", NULLREP)) {
                    strm << ", name \"" << ToString(name) << "\"";
                }
                strm << std::endl;
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Show IBDA_Topology for a COM object
//-----------------------------------------------------------------------------

namespace {
    void DisplayBDATopology(std::ostream& strm, const std::string& margin, ::IUnknown* object, ts::ReportInterface& report)
    {
        using namespace ts;

        // Check if the filter supports IBDA_Topology
        ComPtr <::IBDA_Topology> topology;
        topology.queryInterface(object, ::IID_IBDA_Topology, NULLREP);
        if (topology.isNull()) {
            return;
        }
        strm << margin << "IBDA_Topology:" << std::endl;

        static const ::ULONG MAX_NODES = 64;
        ::ULONG count;
        ::HRESULT hr;

        // Get node descriptors
        ::BDANODE_DESCRIPTOR desc [MAX_NODES];
        count = MAX_NODES;
        hr = topology->GetNodeDescriptors(&count, MAX_NODES, desc);
        if (!ComSuccess(hr, "IBDA_Topology::GetNodeDescriptors", report)) {
            return;
        }
        strm << margin << "  Node descriptors:" << std::endl;
        for (::ULONG node = 0; node < count; node++) {
            strm << margin << "    type " << desc[node].ulBdaNodeType
                << ": function " << NameGUID(desc[node].guidFunction)
                << ", name " << NameGUID(desc[node].guidName)
                << std::endl;
        }

        // Get node types
        ::ULONG types [MAX_NODES];
        count = MAX_NODES;
        hr = topology->GetNodeTypes(&count, MAX_NODES, types);
        if (!ComSuccess(hr, "IBDA_Topology::GetNodeTypes", report)) {
            return;
        }
        for (::ULONG node = 0; node < count; node++) {
            strm << margin << "  Node type " << types[node] << ":" << std::endl;

            // List all interfaces for this node
            ::GUID interfaces [MAX_NODES];
            ::ULONG interfaces_count = MAX_NODES;
            hr = topology->GetNodeInterfaces(types[node], &interfaces_count, MAX_NODES, interfaces);
            if (ComSuccess(hr, "IBDA_Topology::GetNodeInterfaces", report)) {
                for (::ULONG n = 0; n < interfaces_count; n++) {
                    strm << margin << "    interface " << NameGUID(interfaces[n]) << std::endl;
                }
            }

            // Get control node for this type
            ComPtr <::IUnknown> cnode;
            hr = topology->GetControlNode(0, 1, types[node], cnode.creator());
            if (ComSuccess(hr, "IBDA_Topology::GetControlNode", report)) {
                DisplayObject(strm, margin + "    ", cnode, report);
            }
        }

        // Get pin types
        count = MAX_NODES;
        hr = topology->GetPinTypes(&count, MAX_NODES, types);
        if (!ComSuccess(hr, "IBDA_Topology::GetPinTypes", report)) {
            return;
        }
        strm << margin << "  Pin types:";
        if (count <= 0) {
            strm << " none";
        }
        else {
            for (::ULONG n = 0; n < count; n++) {
                strm << " " << types[n];
            }
        }
        strm << std::endl;

        // Get template connections
        ::BDA_TEMPLATE_CONNECTION conn [MAX_NODES];
        count = MAX_NODES;
        hr = topology->GetTemplateConnections(&count, MAX_NODES, conn);
        if (!ComSuccess(hr, "IBDA_Topology::GetTemplateConnections", report)) {
            return;
        }

        strm << margin << "  Template connections:" << std::endl;
        for (::ULONG n = 0; n < count; n++) {
            strm << margin << "    node type " << conn[n].FromNodeType
                << " / pin type " << conn[n].FromNodePinType
                << " -> node type " << conn[n].ToNodeType
                << " / pin type " << conn[n].ToNodePinType
                << std::endl;
        }
    }
}


//-----------------------------------------------------------------------------
// Show selected properties of a COM object
//-----------------------------------------------------------------------------

namespace {
    template <class COMCLASS>
    void DisplayObject(std::ostream& strm, const std::string& margin, const ts::ComPtr<COMCLASS>& obj, ts::ReportInterface& report)
    {
        strm << margin << "Some supported interfaces:" << std::endl;
        DisplayInterfaces(strm, margin + "  ", obj, report);
        DisplayIKsPropertySet(strm, margin, obj.pointer(), report);
        DisplayIKsControl(strm, margin, obj.pointer(), report);
        DisplayIKsTopologyInfo(strm, margin, obj.pointer(), report);
        DisplayBDATopology(strm, margin, obj.pointer(), report);
    }
}


//-----------------------------------------------------------------------------
// Display the description of a DirectShow filter graph.
//-----------------------------------------------------------------------------

bool ts::DisplayFilterGraph(std::ostream& strm,
                            const ComPtr<::IGraphBuilder>& graph,
                            const std::string& margin,
                            bool verbose,
                            ReportInterface& report)
{
    // Find the first filter in a graph: enumerate all filters
    // and get the first one with no connected input pin.
    ComPtr<::IEnumFilters> enum_filters;
    ::HRESULT hr = graph->EnumFilters(enum_filters.creator());
    if (!ComSuccess(hr, "IFilterGraph::EnumFilters", report)) {
        return false;
    }
    ComPtr <::IBaseFilter> filter;
    PinPtrVector pins;
    while (enum_filters->Next(1, filter.creator(), NULL) == S_OK) {
        if (!GetPin(pins, filter.pointer(), xPIN_INPUT | xPIN_CONNECTED, report)) {
            return false;
        }
        if (pins.empty()) {
            // Found one without connected input pin, this is a starting point of the graph
            return DisplayFilterGraph(strm, filter, margin, verbose, report);
        }
    }
    // Found no starting point (empty graph?)
    return true;
}

bool ts::DisplayFilterGraph(std::ostream& strm, const
                            ComPtr<::IBaseFilter>& start_filter,
                            const std::string& margin,
                            bool verbose,
                            ReportInterface& report)
{
    ComPtr<::IBaseFilter> filter(start_filter);

    // Loop on all filters in the graph
    while (!filter.isNull()) {

        // Get filter name
        ::FILTER_INFO filter_info;
        ::HRESULT hr = filter->QueryFilterInfo(&filter_info);
        if (!ComSuccess(hr, "IBaseFilter::QueryFilterInfo", report)) {
            return false;
        }
        filter_info.pGraph->Release();
        std::string filter_name(ToString(filter_info.achName));

        // Get filter vendor info (may be unimplemented)
        std::string filter_vendor;
        ::WCHAR* wstring;
        hr = filter->QueryVendorInfo(&wstring);
        if (SUCCEEDED(hr)) {
            filter_vendor = ToString(wstring);
            ::CoTaskMemFree(wstring);
        }

        // Get filter class GUID if persistent
        ::CLSID class_id(GUID_NULL);
        ComPtr <::IPersist> persist;
        persist.queryInterface(filter.pointer(), ::IID_IPersist, NULLREP);
        if (!persist.isNull()) {
            // Filter class implements IPersist
            hr = persist->GetClassID(&class_id);
            if (!ComSuccess(hr, "get filter class guid", report)) {
                return false;
            }
        }

        // Get connected output pins
        PinPtrVector pins;
        if (!GetPin(pins, filter.pointer(), xPIN_OUTPUT | xPIN_CONNECTED, report)) {
            return false;
        }

        // Display the filter info
        char bar = pins.size() > 1 ? '|' : ' ';
        if (verbose) {
            strm << margin << std::endl;
        }
        strm << margin << "- Filter \"" << filter_name << "\"" << std::endl;
        if (verbose) {
            if (!filter_vendor.empty()) {
                strm << margin << bar << " vendor: \"" << filter_vendor << "\"" << std::endl;
            }
            strm << margin << bar << " class GUID: " << NameGUID(class_id) << std::endl;
            DisplayObject(strm, margin + bar + " ", filter, report);
        }

        // Loop on all connected output pins
        for (size_t pin_index = 0; pin_index < pins.size(); ++pin_index) {
            const ComPtr <::IPin>& output(pins[pin_index]);

            // If more than one output pin, we need to indent and recurse
            bool last_pin = pin_index == pins.size() - 1;
            std::string margin0(margin);
            std::string margin1(margin);
            std::string margin2(margin);
            if (pins.size() > 1) {
                margin0 += "|";
                margin1 += "+--";
                margin2 += last_pin ? "   " : "|  ";
            }

            // Get output pin info
            ::PIN_INFO pin_info;
            hr = output->QueryPinInfo(&pin_info);
            if (!ComSuccess(hr, "IPin::QueryPinInfo", report)) {
                return false;
            }
            std::string pin_name(ToString(pin_info.achName));
            pin_info.pFilter->Release();

            // Get output pin id
            ::WCHAR* wid;
            hr = output->QueryId(&wid);
            if (!ComSuccess(hr, "IPin::QueryPinId", report)) {
                return false;
            }
            std::string pin_id(ToString(wid));
            ::CoTaskMemFree(wid);

            // Display output pin info
            if (verbose) {
                strm << margin0 << std::endl;
            }
            strm << margin1 << "- Output pin \"" << pin_name << "\", id \"" << pin_id << "\"" << std::endl;
            if (verbose) {
                DisplayObject(strm, margin2 + "  ", output, report);
            }

            // Get connection media type
            ::AM_MEDIA_TYPE media;
            hr = output->ConnectionMediaType(&media);
            if (!ComSuccess(hr, "IPin::ConnectionMediaType", report)) {
                return false;
            }

            // Display media type (and free its resources)
            if (verbose) {
                strm << margin2 << std::endl
                     << margin2 << "- Media major type " << NameGUID(media.majortype) << std::endl
                     << margin2 << "  subtype " << NameGUID(media.subtype) << std::endl
                     << margin2 << "  format " << NameGUID(media.formattype) << std::endl;
            }
            else {
                strm << margin2 << "- Media type "
                     << NameGUID(media.majortype) << " / "
                     << NameGUID(media.subtype) << std::endl;
            }
            FreeMediaType(media);

            // Get connected pin (input pin of next filter)
            ComPtr <::IPin> input;
            hr = output->ConnectedTo(input.creator());
            if (!ComSuccess(hr, "IPin::ConnectedTo", report)) {
                return false;
            }

            // Get next input pin info
            hr = input->QueryPinInfo(&pin_info);
            if (!ComSuccess(hr, "IPin::QueryPinInfo", report)) {
                return false;
            }
            pin_name = ToString(pin_info.achName);
            filter = pin_info.pFilter; // next filter in the chain

            // Get input pin id
            hr = input->QueryId(&wid);
            if (!ComSuccess(hr, "IPin::QueryPinId", report)) {
                return false;
            }
            pin_id = ToString(wid);
            ::CoTaskMemFree(wid);

            // Display input pin info
            if (verbose) {
                strm << margin2 << std::endl;
            }
            strm << margin2 << "- Input pin \"" << pin_name << "\", id \"" << pin_id << "\"" << std::endl;
            if (verbose) {
                DisplayObject(strm, margin2 + "  ", input, report);
            }

            // If more than one branch, recurse
            if (pins.size() > 1 && !DisplayFilterGraph(strm, filter, margin2, verbose, report)) {
                return false;
            }
        }
        if (pins.size() != 1) {
            // no connected output pin, end of graph, or more than one and we recursed.
            break;
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Display all devices of the specified category
// Return false on error.
//-----------------------------------------------------------------------------

bool ts::DisplayDevicesByCategory(std::ostream& strm,
                                  const ::GUID& category,
                                  const std::string& margin,
                                  const std::string& name,
                                  ReportInterface& report)
{
    ::HRESULT hr;

    strm << std::endl << margin << "=== Device category " << name << std::endl;

    // Create a DirectShow System Device Enumerator
    ComPtr <::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Enumerate all devices for this category
    ComPtr <::IEnumMoniker> enum_moniker;
    hr = enum_devices->CreateClassEnumerator(category, enum_moniker.creator(), 0);
    if (!ComSuccess(hr, "CreateClassEnumerator", report)) {
        return false;
    }
    if (hr != S_OK) {
        // Empty category, not an error.
        return true;
    }

    // Loop on all enumerated devices.
    ComPtr <::IMoniker> device_moniker;
    while (enum_moniker->Next(1, device_moniker.creator(), NULL) == S_OK) {

        // Get friendly name of this device filter
        std::string device_name(GetStringPropertyBag(device_moniker.pointer(), L"FriendlyName", report));
        strm << std::endl << margin << "device \"" << device_name << "\"" << std::endl;

        // Create an instance of this device from moniker
        ComPtr <::IBaseFilter> filter;
        filter.bindToObject(device_moniker.pointer(), ::IID_IBaseFilter, report);
        if (filter.isNull()) {
            continue;
        }
        DisplayObject(strm, margin + "  ", filter, report);

        // List all pins on the filter
        // Create a pin enumerator
        ComPtr <::IEnumPins> enum_pins;
        hr = filter->EnumPins(enum_pins.creator());
        if (!ComSuccess(hr, "IBaseFilter::EnumPins", report)) {
            return false;
        }

        // Loop on all pins
        ComPtr <::IPin> pin;
        while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {

            // Query direction of this pin
            ::PIN_DIRECTION dir;
            hr = pin->QueryDirection(&dir);
            if (!ComSuccess(hr, "IPin::QueryDirection", report)) {
                return false;
            }
            std::string direction;
            switch (dir) {
                case ::PINDIR_INPUT:  direction = "input"; break;
                case ::PINDIR_OUTPUT: direction = "output"; break;
                default:              direction = Decimal(int(dir));
            }

            // Get pin info
            ::PIN_INFO pin_info;
            hr = pin->QueryPinInfo(&pin_info);
            if (!ComSuccess(hr, "IPin::QueryPinInfo", report)) {
                return false;
            }
            std::string pin_name(ToString(pin_info.achName));
            pin_info.pFilter->Release();

            strm << std::endl << margin << "  - Pin \"" << pin_name << "\", direction: " << direction << std::endl;
            DisplayObject(strm, margin + "    ", pin, report);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Map a DirectShow network provider class id to a tuner type.
// Return false if no match found.
//-----------------------------------------------------------------------------

bool ts::NetworkProviderToTunerType(const ::GUID provider_clsid, TunerType& tuner_type)
{
    if (provider_clsid == ::CLSID_DVBTNetworkProvider) {
        tuner_type = ts::DVB_T;
        return true;
    }
    else if (provider_clsid == ::CLSID_DVBSNetworkProvider) {
        tuner_type = ts::DVB_S;
        return true;
    }
    else if (provider_clsid == ::CLSID_DVBCNetworkProvider) {
        tuner_type = ts::DVB_C;
        return true;
    }
    else if (provider_clsid == ::CLSID_ATSCNetworkProvider) {
        tuner_type = ts::ATSC;
        return true;
    }
    else {
        return false;
    }
}


//-----------------------------------------------------------------------------
// Enumerate all devices of the specified class.
// Fill a vector of monikers to these objects.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::EnumerateDevicesByClass(const ::CLSID& clsid, std::vector <ComPtr <::IMoniker>>& monikers, ReportInterface& report)
{
    // Reset content of vector
    monikers.clear();

    // Create a DirectShow System Device Enumerator
    ComPtr <::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Enumerate devices
    ComPtr <::IEnumMoniker> enum_monikers;
    ::HRESULT hr = enum_devices->CreateClassEnumerator(clsid, enum_monikers.creator(), 0);
    if (!ComSuccess(hr, "CreateClassEnumerator", report)) {
        return false;
    }
    if (hr != S_OK) {
        return true; // empty category
    }

    // Loop on all enumerated providers.
    ComPtr <::IMoniker> moniker;
    while (enum_monikers->Next(1, moniker.creator(), NULL) == S_OK) {
        monikers.push_back(moniker);
    }

    return true;
}


//-----------------------------------------------------------------------------
// Get names of a tuning space. Return empty string on error.
//-----------------------------------------------------------------------------

namespace {
    std::string ToStringAndFree(::HRESULT hr, ::BSTR& name, const char* message, ts::ReportInterface& report)
    {
        std::string cname;
        if (ts::ComSuccess(hr, message, report)) {
            cname = ts::ToString(name);
        }
        if (name != NULL) {
            ::SysFreeString(name);
            name = NULL;
        }
        return cname;
    }
}

std::string ts::GetTuningSpaceFriendlyName(::ITuningSpace* tspace, ReportInterface& report)
{
    ::BSTR name = NULL;
    return ToStringAndFree(tspace->get_FriendlyName(&name), name, "ITuningSpace::get_FriendlyName", report);
}

std::string ts::GetTuningSpaceUniqueName(::ITuningSpace* tspace, ReportInterface& report)
{
    ::BSTR name = NULL;
    return ToStringAndFree(tspace->get_UniqueName(&name), name, "ITuningSpace::get_UniqueName", report);
}
