//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
//  Describe a DVB service
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsVariable.h"
#include "tsStringUtils.h"

namespace ts {

    class TSDUCKDLL Service
    {
    public:
        // Default constructor
        Service() {}

        // Constructor using a string description.
        // If the string evaluates to an integer (decimal or hexa),
        // this is a service id, otherwise this is a service name.
        Service (const std::string& desc) {set (desc);}

        // Reset using a string description.
        // If the string evaluates to an integer (decimal or hexa),
        // this is a service id, otherwise this is a service name.
        void set (const std::string&);

        // Clear all fields
        void clear();

        // Service id
        bool hasId() const {return _id.set();}
        uint16_t getId() const {return _id.set() ? _id.value() : 0;}
        bool hasId (uint16_t id) const {return _id == id;}
        void setId (uint16_t id) {_id = id;}
        void clearId() {_id.reset();}

        // Transport stream id
        bool hasTSId() const {return _tsid.set();}
        uint16_t getTSId() const {return _tsid.set() ? _tsid.value() : 0;}
        bool hasTSId (uint16_t tsid) const {return _tsid == tsid;}
        void setTSId (uint16_t tsid) {_tsid = tsid;}
        void clearTSId() {_tsid.reset();}

        // Original network id
        bool hasONId() const {return _onid.set();}
        uint16_t getONId() const {return _onid.set() ? _onid.value() : 0;}
        bool hasONId (uint16_t onid) const {return _onid == onid;}
        void setONId (uint16_t onid) {_onid = onid;}
        void clearONId() {_onid.reset();}

        // PMT PID
        bool hasPMTPID() const {return _pmt_pid.set();}
        PID getPMTPID() const {return _pmt_pid.set() ? _pmt_pid.value() : PID (PID_NULL);}
        bool hasPMTPID (PID pid) const {return _pmt_pid == pid;}
        void setPMTPID (PID pmt_pid) {_pmt_pid = pmt_pid;}
        void clearPMTPID() {_pmt_pid.reset();}

        // Logical channel number
        bool hasLCN() const {return _lcn.set();}
        uint16_t getLCN() const {return _lcn.set() ? _lcn.value() : 0;}
        bool hasLCN (uint16_t lcn) const {return _lcn == lcn;}
        void setLCN (uint16_t lcn) {_lcn = lcn;}
        void clearLCN() {_lcn.reset();}

        // Service type (as defined in service_descriptor)
        bool hasType() const {return _type.set();}
        uint8_t getType() const {return _type.set() ? _type.value() : 0;}
        bool hasType (uint8_t type) const {return _type == type;}
        void setType (uint8_t type) {_type = type;}
        void clearType() {_type.reset();}

        // Service name
        bool hasName() const {return _name.set();}
        std::string getName() const {return _name.set() ? _name.value() : "";}
        bool hasName (const std::string& name) const {return _name.set() && SimilarStrings (name, _name.value());}
        void setName (const std::string& name) {_name = name;}
        void clearName() {_name.reset();}

        // Provider name
        bool hasProvider() const {return _provider.set();}
        std::string getProvider() const {return _provider.set() ? _provider.value() : "";}
        bool hasProvider (const std::string& provider) const {return _provider.set() && SimilarStrings (provider, _provider.value());}
        void setProvider (const std::string& provider) {_provider = provider;}
        void clearProvider() {_provider.reset();}

        // EIT schedule present (as declared in the SDT)
        bool hasEITsPresent() const {return _eits_present.set();}
        bool getEITsPresent() const {return _eits_present.set() ? _eits_present.value() : false;}
        bool hasEITsPresent (bool eits_present) const {return _eits_present == eits_present;}
        void setEITsPresent (bool eits_present) {_eits_present = eits_present;}
        void clearEITsPresent() {_eits_present.reset();}

        // EIT present/following present (as declared in the SDT)
        bool hasEITpfPresent() const {return _eitpf_present.set();}
        bool getEITpfPresent() const {return _eitpf_present.set() ? _eitpf_present.value() : false;}
        bool hasEITpfPresent (bool eitpf_present) const {return _eitpf_present == eitpf_present;}
        void setEITpfPresent (bool eitpf_present) {_eitpf_present = eitpf_present;}
        void clearEITpfPresent() {_eitpf_present.reset();}

        // CA-controlled (as declared in the SDT)
        bool hasCAControlled() const {return _ca_controlled.set();}
        bool getCAControlled() const {return _ca_controlled.set() ? _ca_controlled.value() : false;}
        bool hasCAControlled (bool ca_controlled) const {return _ca_controlled == ca_controlled;}
        void setCAControlled (bool ca_controlled) {_ca_controlled = ca_controlled;}
        void clearCAControlled() {_ca_controlled.reset();}

        // Running status (as declared in the SDT)
        bool hasRunningStatus() const {return _running_status.set();}
        uint8_t getRunningStatus() const {return _running_status.set() ? _running_status.value() : 0;}
        bool hasRunningStatus (uint8_t running_status) const {return _running_status == running_status;}
        void setRunningStatus (uint8_t running_status) {_running_status = running_status & 0x07;}
        void clearRunningStatus() {_running_status.reset();}

        // List of possible fields a Service may have set.
        // Can be used as bitfield.
        enum {
            ID       = 0x0001,
            TSID     = 0x0002,
            ONID     = 0x0004,
            PMT_PID  = 0x0008,
            LCN      = 0x0010,
            TYPE     = 0x0020,
            NAME     = 0x0040,
            PROVIDER = 0x0080,
            EITS     = 0x0100,
            EITPF    = 0x0200,
            CA       = 0x0400,
            RUNNING  = 0x0800,
        };

        // List of fields which are set in a Service
        uint32_t getFields() const;

        // Sorting criterion using LCN, ONId, TSId, Id, name, provider, type, PMT PID
        static bool Sort1 (const Service&, const Service&);

        // Sorting criterion using name, provider, LCN, ONId, TSId, Id, type, PMT PID
        static bool Sort2 (const Service&, const Service&);

        // Sorting criterion using ONId, TSId, Id, type, name, provider, LCN, PMT PID
        static bool Sort3 (const Service&, const Service&);

        // Display a container of services.
        template <class ITERATOR>
        static std::ostream& Display (std::ostream&,
                                      const std::string& margin,
                                      const ITERATOR& begin,
                                      const ITERATOR& end,
                                      bool header = true);

        template <class CONTAINER>
        static std::ostream& Display (std::ostream& strm, const std::string& margin, const CONTAINER& container, bool header = true)
        {
            return Display (strm, margin, container.begin(), container.end(), header);
        }

    private:
        Variable <uint16_t>      _id;
        Variable <uint16_t>      _tsid;
        Variable <uint16_t>      _onid;
        Variable <PID>         _pmt_pid;
        Variable <uint16_t>      _lcn;
        Variable <uint8_t>       _type;
        Variable <std::string> _name;
        Variable <std::string> _provider;
        Variable <bool>        _eits_present;
        Variable <bool>        _eitpf_present;
        Variable <bool>        _ca_controlled;
        Variable <uint8_t>       _running_status;
    };

    // Containers
    typedef std::vector<Service> ServiceVector;
    typedef std::list<Service> ServiceList;
    typedef std::set<Service> ServiceSet;
}

#include "tsServiceTemplate.h"
