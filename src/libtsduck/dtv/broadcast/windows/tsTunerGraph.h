//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsNullReport.h"

namespace ts {
    //!
    //! A specialization of a DirectShow graph for tuner reception (Windows-specific).
    //! @ingroup libtsduck windows
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
        TunerGraph() = default;

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
            psFIRST,    //!< Keep first value, when found.
            psLAST,     //!< Keep last value.
            psLOWEST,   //!< Keep lowest value.
            psHIGHEST   //!< Keep highest value.
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
        template <typename VALTYPE, typename IVALTYPE, class FILTER,
                  typename std::enable_if<std::is_base_of<::IBDA_SignalStatistics, FILTER>::value>::type* = nullptr>
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
        template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER,
                  typename std::enable_if<std::is_same<::IBDA_DigitalDemodulator, FILTER>::value>::type* = nullptr>
        bool searchVarProperty(VALTYPE unset,
                               std::optional<ARGTYPE>& parameter,
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
        template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER,
                  typename std::enable_if<std::is_same<::IBDA_DigitalDemodulator2, FILTER>::value>::type* = nullptr>
        bool searchVarProperty(VALTYPE unset,
                               std::optional<ARGTYPE>& parameter,
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
        UString                                         _user_receiver_name {};  // User-specified receiver filter name.
        UString                                         _tuner_name {};          // Name of the tuner filter.
        ComPtr<SinkFilter>                              _sink_filter {};         // Sink filter to TSDuck
        ComPtr<::IBaseFilter>                           _provider_filter {};     // Network provider filter
        ComPtr<::IBDA_NetworkProvider>                  _inet_provider {};       // ... interface of _provider_filter
        ComPtr<::ITuner>                                _ituner {};              // ... interface of _provider_filter
        ComPtr<::ITunerCap>                             _ituner_cap {};          // ... interface of _provider_filter
        std::map<TunerType, DirectShowNetworkType>      _net_types {};           // Map of network types for this tuner.
        ComPtr<::IBaseFilter>                           _tuner_filter {};        // Tuner filter
        std::vector<ComPtr<::IBDA_DigitalDemodulator>>  _demods {};              // ... all its demod interfaces
        std::vector<ComPtr<::IBDA_DigitalDemodulator2>> _demods2 {};             // ... all its demod (2nd gen) interfaces
        std::vector<ComPtr<::IBDA_SignalStatistics>>    _sigstats {};            // ... all its signal stat interfaces
        std::vector<ComPtr<::IKsPropertySet>>           _tunprops {};            // ... all its property set interfaces

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
                                   std::optional<ARGTYPE>& parameter,
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


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Locate all known interfaces in a pin or node of the tuner filter.
template <class COMCLASS>
void ts::TunerGraph::findTunerSubinterfaces(ComPtr<COMCLASS>& obj)
{
    findTunerSubinterface(obj, IID_IBDA_DigitalDemodulator,  _demods);
    findTunerSubinterface(obj, IID_IBDA_DigitalDemodulator2, _demods2);
    findTunerSubinterface(obj, IID_IBDA_SignalStatistics,    _sigstats);
    findTunerSubinterface(obj, IID_IKsPropertySet,           _tunprops);
}

// Locate one interface in a pin or node of the tuner filter.
template <class COMCLASS, class IFACE>
void ts::TunerGraph::findTunerSubinterface(ComPtr<COMCLASS>& obj, const IID& interface_id, std::vector<ComPtr<IFACE>>& ivector)
{
    ComPtr<IFACE> iobj;
    iobj.queryInterface(obj.pointer(), interface_id, NULLREP);
    if (!iobj.isNull()) {
        ivector.push_back(iobj);
    }
}

// Repeatedly called when searching for a propery.
template <typename T>
void ts::TunerGraph::SelectProperty(bool& terminated, bool& found, T& retvalue, T val, PropSearch searchtype)
{
    switch (searchtype) {
        case psFIRST:
            retvalue = val;
            terminated = true;
            break;
        case psLAST:
            retvalue = val;
            break;
        case psHIGHEST:
            if (!found || val > retvalue) {
                retvalue = val;
            }
            break;
        case psLOWEST:
            if (!found || val < retvalue) {
                retvalue = val;
            }
            break;
    }
    found = true;
}

// Search all IKsPropertySet in the tuner until the specified data is found.
template <typename VALTYPE>
bool ts::TunerGraph::searchTunerProperty(VALTYPE& retvalue, PropSearch searchtype, const ::GUID& propset, int propid)
{
    bool found = false;
    bool terminated = false;

    // Loop on all property set interfaces in the tuner filter.
    for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {
        VALTYPE val = VALTYPE(0);
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(_tunprops[i]->Get(propset, propid, nullptr, 0, &val, retsize, &retsize))) {
            SelectProperty(terminated, found, retvalue, val, searchtype);
        }
    }
    return found;
}

// Search a property, until found, in "ivector" and then _tunprops.
template <typename VALTYPE, typename IVALTYPE, class FILTER>
bool ts::TunerGraph::searchPropertyImpl(VALTYPE& retvalue,
                                        PropSearch searchtype,
                                        const std::vector<ComPtr<FILTER>>& ivector,
                                        ::HRESULT (__stdcall FILTER::*getmethod)(IVALTYPE*),
                                        const ::GUID& propset,
                                        int propid)
{
    bool found = false;
    bool terminated = false;

    // First step, lookup all interfaces of a given type.
    for (size_t i = 0; !terminated && i < ivector.size(); ++i) {
        IVALTYPE val;
        FILTER* filter = ivector[i].pointer();
        if (SUCCEEDED((filter->*getmethod)(&val))) {
            SelectProperty<VALTYPE>(terminated, found, retvalue, val, searchtype);
        }
    }

    // Second step, lookup tuner properties.
    for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {
        VALTYPE val;
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(_tunprops[i]->Get(propset, propid, nullptr, 0, &val, retsize, &retsize))) {
            SelectProperty<VALTYPE>(terminated, found, retvalue, val, searchtype);
        }
    }

    return found;
}

// Same one with additional handling of unknown return value.
template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER>
bool ts::TunerGraph::searchVarPropertyImpl(VALTYPE unset,
                                           std::optional<ARGTYPE>& parameter,
                                           PropSearch searchtype,
                                           bool reset_unknown,
                                           const std::vector<ComPtr<FILTER>>& ivector,
                                           ::HRESULT (__stdcall FILTER::*getmethod)(IVALTYPE*),
                                           const ::GUID& propset,
                                           int propid)
{
    VALTYPE retvalue = unset;
    bool found = searchPropertyImpl(retvalue, searchtype, ivector, getmethod, propset, propid);
    if (found && retvalue != unset) {
        parameter = ARGTYPE(retvalue);
    }
    else if (reset_unknown) {
        parameter.reset();
    }
    return found;
}
