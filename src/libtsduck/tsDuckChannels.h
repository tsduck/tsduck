//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  XML files containing the description of TV channels and their networks.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerParameters.h"
#include "tsTunerParametersATSC.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBT.h"
#include "tsxmlDocument.h"
#include "tsxmlTweaks.h"
#include "tsCerrReport.h"
#include "tsSafePtr.h"
#include "tsVariable.h"
#include "tsMPEG.h"

namespace ts {
    //!
    //! An XML file containing the description of TV channels and their networks.
    //! @ingroup mpeg
    //!
    //! The XML template for such files is in file @c tsduck.channels.xml.
    //! This is a proprietary XML syntax from TSDuck which is used to replace the
    //! deprecated "zap" format from Linux tools such @c szap or @c tzap.
    //!
    //! The default file location depends on the operating system:
    //! - Windows: @c \%APPDATA%\\tsduck\\channels.xml
    //! - Unix: @c $HOME/.tsduck.channels.xml
    //!
    class TSDUCKDLL DuckChannels
    {
    public:
        //!
        //! Default constructor.
        //!
        DuckChannels();

        //!
        //! Set new parsing and formatting tweaks for XML files.
        //! @param [in] tweaks XML tweaks.
        //!
        void setTweaks(const xml::Tweaks& tweaks) { _xmlTweaks = tweaks; }

        //!
        //! Clear the loaded content.
        //!
        void clear();

        //!
        //! Load an XML file.
        //! @param [in] fileName XML file name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool load(const UString& fileName, Report& report = CERR);

        //!
        //! Load an XML file.
        //! @param [in,out] strm A standard text stream in input mode.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool load(std::istream& strm, Report& report = CERR);

        //!
        //! Parse an XML content.
        //! @param [in] text XML file content.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool parse(const UString& text, Report& report = CERR);

        //!
        //! Save an XML file.
        //! @param [in] fileName XML file name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool save(const UString& fileName, Report& report = CERR) const;

        //!
        //! Serialize as XML text.
        //! @param [in,out] report Where to report errors.
        //! @return Complete XML document text, empty on error.
        //!
        UString toXML(Report& report = CERR) const;

        //!
        //! Default XML channel file name.
        //! @return The default file name.
        //!
        static UString DefaultFileName();

        //!
        //! Description of one service.
        //!
        class TSDUCKDLL Service
        {
        public:
            Service();                    //!< Default constructor.
            uint16_t           id;        //!< Service Id.
            UString            name;      //!< Service Name.
            UString            provider;  //!< Provider Name.
            Variable<uint16_t> lcn;       //!< Logical Channel Number (optional).
            Variable<PID>      pmtPID;    //!< PMT PID (optional).
            Variable<uint8_t>  type;      //!< Service Type as declared in service_descriptor (optional).
            Variable<bool>     cas;       //!< CA-controlled as declared in the SDT (optional).
        };

        typedef SafePtr<Service> ServicePtr;        //!< Safe pointer to a Service object.
        typedef std::list<ServicePtr> ServiceList;  //!< List of safe pointers to Service objects.

        //!
        //! Description of one transport stream.
        //!
        class TSDUCKDLL TransportStream
        {
        public:
            TransportStream();            //!< Default constructor.
            uint16_t           id;        //!< Transport Stream Id.
            uint16_t           onid;      //!< Original Network Id.
            TunerParametersPtr tune;      //!< Tuner parameters for the transport stream.
            ServiceList        services;  //!< List of services in the transport stream.
        };

        typedef SafePtr<TransportStream> TransportStreamPtr;        //!< Safe pointer to a TransportStream object.
        typedef std::list<TransportStreamPtr> TransportStreamList;  //!< List of safe pointers to TransportStream objects.

        //!
        //! Description of one network.
        //!
        class TSDUCKDLL Network
        {
        public:
            Network();                 //!< Default constructor.
            uint16_t            id;    //!< Network Id.
            TunerType           type;  //!< Network distribution type (same as tuner type).
            TransportStreamList ts;    //!< List of transport streams in the network.
        };

        typedef SafePtr<Network> NetworkPtr;        //!< Safe pointer to a Network object.
        typedef std::list<NetworkPtr> NetworkList;  //!< List of safe pointers to Network objects.

        // Public members of ts::DuckChannels.
        NetworkList networks;  //<! List of networks in the configuration.
    private:
        xml::Tweaks _xmlTweaks;       //!< XML formatting and parsing tweaks.

        // Parse an XML document and load the content into this object.
        bool parseDocument(const xml::Document& doc);

        // Generate an XML document from the content of this object.
        bool generateDocument(xml::Document& doc) const;

        // Generate an XML element from a set of tuner parameters.
        static void TunerToXml(xml::Element* parent, const TunerParametersATSC* params);
        static void TunerToXml(xml::Element* parent, const TunerParametersDVBC* params);
        static void TunerToXml(xml::Element* parent, const TunerParametersDVBS* params);
        static void TunerToXml(xml::Element* parent, const TunerParametersDVBT* params);

        // Parse an XML element into a set of tuner parameters.
        static bool XmlToATCS(TunerParametersPtr& params, const xml::Element* elem);
        static bool XmlToDVBC(TunerParametersPtr& params, const xml::Element* elem);
        static bool XmlToDVBS(TunerParametersPtr& params, const xml::Element* elem);
        static bool XmlToDVBT(TunerParametersPtr& params, const xml::Element* elem);
    };
}
