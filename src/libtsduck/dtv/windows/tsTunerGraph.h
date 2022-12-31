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
//!
//!  @file
//!  A specialization of a DirectShow graph for tuner reception.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsDirectShowGraph.h"
#include "tsDirectShowNetworkType.h"
#include "tsModulationArgs.h"
#include "tsSinkFilter.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! A specialization of a DirectShow graph for tuner reception (Windows-specific).
    //! @ingroup windows
    //!
    //! A DirectShow graph for TS capture is usually made of the following filters:
    //! - Network provider (typically "Microsoft DVBx Network Provider")
    //! - Tuner (typically provided by tuner hardware vendor as "BDA driver")
    //! - Receiver (optional, also provided by tuner hardware vendor)
    //! - Tee filter, creating two branches:
    //! - Branch A: actual capture of TS packets
    //!   - SinkFiler (provided by TSDuck)
    //! - Branch B: MPEG-2 demux, actually unused but required by the graph
    //!   - MPEG-2 demultiplexer
    //!   - TIF (Transport Information Filter)
    //!
    class TSDUCKDLL TunerGraph : public DirectShowGraph
    {
        TS_NOCOPY(TunerGraph);
    public:
        //!
        //! Default constructor.
        //!
        TunerGraph();

        //!
        //! Destructor.
        //!
        virtual ~TunerGraph();

        //!
        //! Specify a receiver filter name.
        //! Must be called before initialize(). The graph will use the specified receiver
        //! filter instead of the standard algorithm.
        //! @param [in] name Name of the receiver filter to use.
        //!
        void setReceiverName(const UString& name) { _user_receiver_name = name; }

        //!
        //! Initialize the graph.
        //! @param [in] tuner_name Tuner filter name (informational only).
        //! @param [in,out] tuner_moniker A moniker to create instances of a tuner filter.
        //! This tuner filter is the base of the graph creation (not the starting point
        //! of the graph, which is the network provider filter).
        //! @param [out] delivery_systems List of delivery systems which are supported by the tuner.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool initialize(const UString& tuner_name, ::IMoniker* tuner_moniker, DeliverySystemSet& delivery_systems, Report& report);

        // Inherited methods.
        virtual void clear(Report& report) override;

        //!
        //! Get the sink filter of the graph.
        //! This is where the TS packets can be fetched out of the graph.
        //! @return The address of the sink filter or a null pointer if the graph is not initialized.
        //!
        SinkFilter* sinkFilter() const { return _sink_filter.pointer(); }

        //!
        //! Send a tune request.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] params Modulation parameters.
        //! @return True on success, false on error.
        //!
        bool sendTuneRequest(DuckContext& duck, const ModulationArgs& params);

        //!
        //! Search criteria for properties.
        //!
        enum PropSearch {
            psFIRST,    //! Keep first value, when found.
            psLAST,     //! Keep last value.
            psLOWEST,   //! Keep lowest value.
            psHIGHEST   //! Keep highest value.
        };

        //!
        //! Search all IKsPropertySet in the tuner until the specified data is found.
        //! @tparam VALTYPE The type of the property to search.
        //! @param [out] retvalue Returned property value.
        //! @param [in] searchtype Selection criteria for the final value.
        //! @param [in] propset GUID of the property set.
        //! @param [in] propid Id of the property to search.
        //! @return True when the property was found, false otherwise.
        //!
        template <typename VALTYPE>
        bool searchTunerProperty(VALTYPE& retvalue, PropSearch searchtype, const ::GUID& propset, int propid);

        //!
        //! Search a property, until found, in all interfaces of a given class and then in tuner properties.
        //! @tparam VALTYPE The type of the property to search.
        //! @tparam IVALTYPE The type of the property in the get_XXX() method of the object.
        //! @tparam FILTER The interface class through which the property can be found.
        //! @param [out] retvalue Returned property value.
        //! @param [in] searchtype Selection criteria for the final value.
        //! @param [in] getmethod The get_XXX() method of the object to retrieve the property. Used to search in @a ivector.
        //! @param [in] propset GUID of the property set. Used to search in tuner properties.
        //! @param [in] propid Id of the property to search.
        //! @return True when the property was found, false otherwise.
        //!
        template <typename VALTYPE, typename IVALTYPE, class FILTER, typename std::enable_if<std::is_base_of<::IBDA_SignalStatistics, FILTER>::value>::type* = nullptr>
        bool searchProperty(VALTYPE& retvalue,
                            PropSearch searchtype,
                            ::HRESULT (__stdcall FILTER::* getmethod)(IVALTYPE*),
                            const ::GUID& propset,
                            int propid)
        {
            return searchPropertyImpl(retvalue, searchtype, _sigstats, getmethod, propset, propid);
        }

        //!
        //! Search a property, until found, in all interfaces of a given class and then in tuner properties.
        //! Same as previous method, with additional handling of unknown return value.
        //! @tparam VALTYPE The type of the property to search.
        //! @tparam ARGTYPE
        //! @tparam IVALTYPE The type of the property in the get_XXX() method of the object.
        //! @tparam FILTER The interface class through which the property can be found.
        //! @param [in] unset A value which, when returned by the interface, means "value is unset".
        //! @param [in,out] parameter A variable object containing the result. When the value is considered as "unset",
        //! the variable is reset (becomes uninitialized).
        //! @param [in] searchtype Selection criteria for the final value.
        //! @param [in] reset_unknown If true and the property cannot be found, reset the variable in @a parameter.
        //! @param [in] getmethod The get_XXX() method of the object to retrieve the property. Used to search in @a ivector.
        //! @param [in] propset GUID of the property set. Used to search in tuner properties.
        //! @param [in] propid Id of the property to search.
        //! @return True when the property was found, false otherwise.
        //!
        template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER, typename std::enable_if<std::is_same<::IBDA_DigitalDemodulator, FILTER>::value>::type* = nullptr>
        bool searchVarProperty(VALTYPE unset,
                               Variable<ARGTYPE>& parameter,
                               PropSearch searchtype,
                               bool reset_unknown,
                               ::HRESULT (__stdcall FILTER::* getmethod)(IVALTYPE*),
                               const ::GUID& propset,
                               int propid)
        {
            return searchVarPropertyImpl(unset, parameter, searchtype, reset_unknown, _demods, getmethod, propset, propid);
        }

#if !defined(DOXYGEN)

        // There is one specialization per interface type.
        template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER, typename std::enable_if<std::is_same<::IBDA_DigitalDemodulator2, FILTER>::value>::type* = nullptr>
        bool searchVarProperty(VALTYPE unset,
                               Variable<ARGTYPE>& parameter,
                               PropSearch searchtype,
                               bool reset_unknown,
                               ::HRESULT (__stdcall FILTER::* getmethod)(IVALTYPE*),
                               const ::GUID& propset,
                               int propid)
        {
            return searchVarPropertyImpl(unset, parameter, searchtype, reset_unknown, _demods2, getmethod, propset, propid);
        }

#endif

    private:
        UString                        _user_receiver_name;  // User-specified receiver filter name.
        UString                        _tuner_name;          // Name of the tuner filter.
        ComPtr<SinkFilter>             _sink_filter;         // Sink filter to TSDuck
        ComPtr<::IBaseFilter>          _provider_filter;     // Network provider filter
        ComPtr<::IBDA_NetworkProvider> _inet_provider;       // ... interface of _provider_filter
        ComPtr<::ITuner>               _ituner;              // ... interface of _provider_filter
        ComPtr<::ITunerCap>            _ituner_cap;          // ... interface of _provider_filter
        std::map<TunerType, DirectShowNetworkType>      _net_types;     // Map of network types for this tuner.
        ComPtr<::IBaseFilter>                           _tuner_filter;  // Tuner filter
        std::vector<ComPtr<::IBDA_DigitalDemodulator>>  _demods;        // ... all its demod interfaces
        std::vector<ComPtr<::IBDA_DigitalDemodulator2>> _demods2;       // ... all its demod (2nd gen) interfaces
        std::vector<ComPtr<::IBDA_SignalStatistics>>    _sigstats;      // ... all its signal stat interfaces
        std::vector<ComPtr<::IKsPropertySet>>           _tunprops;      // ... all its property set interfaces

        //!
        //! Try to build the part of the graph starting at the tee filter.
        //! @param [in] base Base filter, either the tuner filter or some
        //! other intermediate receiver filter downstream the tuner.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool buildGraphAtTee(const ComPtr<::IBaseFilter>& base, Report& report);

        //!
        //! Try to build the end of the graph starting at the Transport Information Filter (TIF), after the demux filter.
        //! @param [in] demux Demux filter. The end of the graph is built from here.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool buildGraphAtTIF(const ComPtr<::IBaseFilter>& demux, Report& report);

        //!
        //! Try to install a Transport Information Filter (TIF), after the demux filter.
        //! @param [in] demux Demux filter (already in the graph).
        //! @param [in] tif The transport information filter to install.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool installTIF(const ComPtr<::IBaseFilter>& demux, const ComPtr<::IBaseFilter>& tif, Report& report);

        //!
        //! Locate all known interfaces in a pin or node of the tuner filter.
        //! Add found interfaces in demods, demods2, _sigstats or _tunprops.
        //! Ignore errors.
        //! @tparam COMCLASS A COM object class to search.
        //! @param [in] obj A pin or internal node of the tuner filter.
        //!
        template <class COMCLASS>
        void findTunerSubinterfaces(ComPtr<COMCLASS>& obj);

        //!
        //! Locate one interface in a pin or node of the tuner filter.
        //! Ignore errors.
        //! @tparam COMCLASS A COM object class to search.
        //! @tparam IFACE A COM interface of the object to search.
        //! @param [in] obj A pin or internal node of the tuner filter.
        //! @param [in] interface_id Id of the interface we request in the object.
        //! @param [in,out] ivector Where to push the interface is found.
        //!
        template <class COMCLASS, class IFACE>
        void findTunerSubinterface(ComPtr<COMCLASS>& obj, const IID& interface_id, std::vector<ComPtr<IFACE>>& ivector);

        //!
        //! Search a property, until found, in all interfaces of a given class and then in tuner properties.
        //! @tparam VALTYPE The type of the property to search.
        //! @tparam IVALTYPE The type of the property in the get_XXX() method of the object.
        //! @tparam FILTER The interface class through which the property can be found.
        //! @param [out] retvalue Returned property value.
        //! @param [in] searchtype Selection criteria for the final value.
        //! @param [in] ivector A vector of FILTER interfaces.
        //! @param [in] getmethod The get_XXX() method of the object to retrieve the property. Used to search in @a ivector.
        //! @param [in] propset GUID of the property set. Used to search in tuner properties.
        //! @param [in] propid Id of the property to search.
        //! @return True when the property was found, false otherwise.
        //!
        template <typename VALTYPE, typename IVALTYPE, class FILTER>
        bool searchPropertyImpl(VALTYPE& retvalue,
                                PropSearch searchtype,
                                const std::vector<ComPtr<FILTER>>& ivector,
                                ::HRESULT (__stdcall FILTER::*getmethod)(IVALTYPE*),
                                const ::GUID& propset,
                                int propid);

        //!
        //! Search a property, until found, in all interfaces of a given class and then in tuner properties.
        //! Same as previous method, with additional handling of unknown return value.
        //! @tparam VALTYPE The type of the property to search.
        //! @tparam ARGTYPE
        //! @tparam IVALTYPE The type of the property in the get_XXX() method of the object.
        //! @tparam FILTER The interface class through which the property can be found.
        //! @param [in] unset A value which, when returned by the interface, means "value is unset".
        //! @param [in,out] parameter A variable object containing the result. When the value is considered as "unset",
        //! the variable is reset (becomes uninitialized).
        //! @param [in] searchtype Selection criteria for the final value.
        //! @param [in] reset_unknown If true and the property cannot be found, reset the variable in @a parameter.
        //! @param [in] ivector A vector of FILTER interfaces.
        //! @param [in] getmethod The get_XXX() method of the object to retrieve the property. Used to search in @a ivector.
        //! @param [in] propset GUID of the property set. Used to search in tuner properties.
        //! @param [in] propid Id of the property to search.
        //! @return True when the property was found, false otherwise.
        //!
        template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER>
        bool searchVarPropertyImpl(VALTYPE unset,
                                   Variable<ARGTYPE>& parameter,
                                   PropSearch searchtype,
                                   bool reset_unknown,
                                   const std::vector<ComPtr<FILTER>>& ivector,
                                   ::HRESULT (__stdcall FILTER::*getmethod)(IVALTYPE*),
                                   const ::GUID& propset,
                                   int propid);

        //!
        //! Repeatedly called when searching for a propery.
        //! Each @a val is proposed until @a terminated is returned as true.
        //! @tparam T The type of the property to search.
        //! @param [out] terminated Set to true when the value is definitely found.
        //! @param [in,out] found Set to true all the time. Used to check that SelectProperty() was called at least once.
        //! @param [in,out] retvalue Set to @a val when @a val is the best value to far. Unchanged otherwise.
        //! @param [in] val val Proposed value.
        //! @param [in] searchtype Selection criteria for the final value.
        //!
        template <typename T>
        static void SelectProperty(bool& terminated, bool& found, T& retvalue, T val, PropSearch searchtype);
    };
}

#include "tsTunerGraphTemplate.h"
