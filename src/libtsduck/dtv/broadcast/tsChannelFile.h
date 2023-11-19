//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  XML files containing the description of TV channels and their networks.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsModulationArgs.h"
#include "tsService.h"
#include "tsxmlDocument.h"
#include "tsxmlTweaks.h"
#include "tsCerrReport.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! An XML file containing the description of TV channels and their networks.
    //! @ingroup mpeg
    //!
    //! The XML template for such files is in file @c tsduck.channels.model.xml.
    //! This is a proprietary XML syntax from TSDuck which is used to replace the
    //! deprecated "zap" format from Linux tools such @c szap or @c tzap.
    //!
    //! The default file location depends on the operating system:
    //! - Windows: @c \%APPDATA%\\tsduck\\channels.xml
    //! - Unix: @c $HOME/.tsduck.channels.xml
    //!
    class TSDUCKDLL ChannelFile
    {
    public:
        //!
        //! Default constructor.
        //!
        ChannelFile() = default;

        //!
        //! Set new parsing and formatting tweaks for XML files.
        //! @param [in] tweaks XML tweaks.
        //!
        void setTweaks(const xml::Tweaks& tweaks) { _xmlTweaks = tweaks; }

        //!
        //! Clear all networks.
        //!
        void clear() { _networks.clear(); }

        //!
        //! Load an XML file.
        //! @param [in] fileName XML file name. If empty, use the default file name.
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
        //! @param [in] createDirectories If true, also create intermediate directories if necessary.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool save(const UString& fileName, bool createDirectories = false, Report& report = CERR) const;

        //!
        //! Serialize as XML text.
        //! @param [in,out] report Where to report errors.
        //! @return Complete XML document text, empty on error.
        //!
        UString toXML(Report& report = CERR) const;

        //!
        //! Get the file name from which the channel database was loaded.
        //! @return The file name. Empty string if no file was loaded.
        //!
        UString fileName() const { return _fileName; }

        //!
        //! Get a description of the file from which the channel database was loaded.
        //! Typically used in error or debug messages.
        //! @return The file name or description. Never empty.
        //!
        UString fileDescription() const;

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
            uint16_t                id = 0;         //!< Service Id.
            UString                 name {};        //!< Service Name.
            UString                 provider {};    //!< Provider Name.
            std::optional<uint16_t> lcn {};         //!< Logical Channel Number (optional).
            std::optional<PID>      pmtPID {};      //!< PMT PID (optional).
            std::optional<uint8_t>  type {};        //!< DVB service type as declared in service_descriptor (optional).
            std::optional<bool>     cas {};         //!< CA-controlled as declared in the SDT (optional).
            std::optional<uint8_t>  atscType {};    //!< ATSC service type as declared in TVCT or CVCT (6 bits, optional).
            std::optional<uint16_t> atscMajorId {}; //!< ATSC service major id as declared in TVCT or CVCT (10 bits, optional).
            std::optional<uint16_t> atscMinorId {}; //!< ATSC service minor id as declared in TVCT or CVCT (10 bits, optional).

            //!
            //! Default constructor.
            //! @param [in] sid Service Id.
            //!
            Service(uint16_t sid = 0) : id(sid) {}
        };

        typedef SafePtr<Service, std::mutex>  ServicePtr;     //!< Safe pointer to a Service object (thread-safe).
        typedef std::vector<ServicePtr> ServiceVector;  //!< Vector of safe pointers to Service objects.

        //!
        //! Description of one transport stream.
        //!
        class TSDUCKDLL TransportStream
        {
        public:
            uint16_t       id = 0;    //!< Transport Stream Id.
            uint16_t       onid = 0;  //!< Original Network Id.
            ModulationArgs tune {};   //!< Tuner parameters for the transport stream.

            //!
            //! Default constructor.
            //! @param [in] ts Transport Stream Id.
            //! @param [in] on Original Network Id.
            //!
            TransportStream(uint16_t ts = 0, uint16_t on = 0) : id(ts), onid(on) {}

            //!
            //! Clear all services.
            //!
            void clear() { _services.clear(); }

            //!
            //! Get the number of services in the transport stream.
            //! @return The number of services in the transport stream.
            //!
            size_t serviceCount() const { return _services.size(); }

            //!
            //! Get a service by index in the transport stream.
            //! @param [in] index Service index, from 0 to serviceCount()-1.
            //! @return A safe pointer to the service description or a null pointer if the specified service does not exist.
            //!
            ServicePtr serviceByIndex(size_t index) const;

            //!
            //! Get a service by id in the transport stream.
            //! @param [in] id Service identifier.
            //! @return A safe pointer to the service description or a null pointer if the specified service does not exist.
            //!
            ServicePtr serviceById(uint16_t id) const;

            //!
            //! Get a service by name in the transport stream.
            //! @param [in] name Service name.
            //! @param [in] strict If true, search exactly @a name.
            //! If false, the comparison is case-insensitive and spaces are ignored.
            //! If false, @a name can also be "major.minor" for ATSC services.
            //! @return A safe pointer to the service description or a null pointer if the specified service does not exist.
            //!
            ServicePtr serviceByName(const UString& name, bool strict = true) const;

            //!
            //! Get or create a service by id in the transport stream.
            //! @param [in] id Service identifier.
            //! @return A safe pointer to the service, never a null pointer.
            //!
            ServicePtr serviceGetOrCreate(uint16_t id);

            //!
            //! Add a service in the transport stream.
            //! @param [in] srv Safe pointer to the new service.
            //! @param [in] copy When COPY, duplicate the service object. When SHARE, simply copy the pointer to the same object.
            //! @param [in] replace If true, replace a service with same id if it already exists.
            //! @return True if the service has been added. False if @a replace is false and a service with same id already exists.
            //!
            bool addService(const ServicePtr& srv, ShareMode copy = ShareMode::SHARE, bool replace = true);

            //!
            //! Add a list of services in the transport stream.
            //! Existing services are updated with new info.
            //! @param [in] list List of services.
            //!
            void addServices(const ServiceList& list);

        private:
            ServiceVector _services {};  // Services in the transport stream.
        };

        typedef SafePtr<TransportStream, std::mutex>  TransportStreamPtr;     //!< Safe pointer to a TransportStream object (thread-safe).
        typedef std::vector<TransportStreamPtr> TransportStreamVector;  //!< Vector of safe pointers to TransportStream objects.

        //!
        //! Description of one network.
        //!
        class TSDUCKDLL Network
        {
        public:
            uint16_t  id;    //!< Network Id.
            TunerType type;  //!< Tuner type (a subset of delivery system).

            //!
            //! Default constructor.
            //! @param [in] net Network Id.
            //! @param [in] typ Tuner type.
            //!
            Network(uint16_t net = 0, TunerType typ = TT_UNDEFINED) : id(net), type(typ) {}

            //!
            //! Clear all transport streams.
            //!
            void clear() { _ts.clear(); }

            //!
            //! Get the number of transport streams in the network.
            //! @return The number of transport streams in the network.
            //!
            size_t tsCount() const { return _ts.size(); }

            //!
            //! Get a transport stream by index in the network.
            //! @param [in] index TS index, from 0 to tsCount()-1.
            //! @return A safe pointer to the TS or a null pointer if the specified TS does not exist.
            //!
            TransportStreamPtr tsByIndex(size_t index) const;

            //!
            //! Get a transport stream by id in the network.
            //! @param [in] id TS identifier.
            //! @return A safe pointer to the TS or a null pointer if the specified TS does not exist.
            //!
            TransportStreamPtr tsById(uint16_t id) const;

            //!
            //! Get or create a transport stream by id in the network.
            //! @param [in] id TS identifier.
            //! @return A safe pointer to the TS, never a null pointer.
            //!
            TransportStreamPtr tsGetOrCreate(uint16_t id);

        private:
            TransportStreamVector _ts {};  // Transport streams in the network.
        };

        typedef SafePtr<Network, std::mutex>  NetworkPtr;     //!< Safe pointer to a Network object (thread-safe).
        typedef std::vector<NetworkPtr> NetworkVector;  //!< Vector of safe pointers to Network objects.

        //!
        //! Get the number of networks in the file.
        //! @return The number of networks in the file.
        //!
        size_t networkCount() const { return _networks.size(); }

        //!
        //! Get a network by index in the file.
        //! @param [in] index Network index, from 0 to networkCount()-1.
        //! @return A safe pointer to the network or a null pointer if the specified network does not exist.
        //!
        NetworkPtr networkByIndex(size_t index) const;

        //!
        //! Get a network by id and type in the file.
        //! @param [in] id Network identifier.
        //! @param [in] type Network type.
        //! @return A safe pointer to the network or a null pointer if the specified network does not exist.
        //!
        NetworkPtr networkById(uint16_t id, TunerType type) const;

        //!
        //! Get or create a network.
        //! @param [in] id Network identifier.
        //! @param [in] type Network type.
        //! @return A safe pointer to the network, never a null pointer.
        //!
        NetworkPtr networkGetOrCreate(uint16_t id, TunerType type);

        //!
        //! Search a service by name in any network of the file.
        //! @param [out] net Returned network of the service.
        //! @param [out] ts Returned transport stream of the service.
        //! @param [out] srv Returned service.
        //! @param [in] name Service name.
        //! @param [in] strict If true, search exactly @a name.
        //! If false, the comparison is case-insensitive and spaces are ignored.
        //! If false, @a name can also be "major.minor" for ATSC services.
        //! @param [in,out] report Where to report errors.
        //! @return True if the service is found, false if the specified service does not exist.
        //!
        bool searchService(NetworkPtr& net,
                           TransportStreamPtr& ts,
                           ServicePtr& srv,
                           const UString& name,
                           bool strict = true,
                           Report& report = CERR) const
        {
            return searchService(net, ts, srv, DeliverySystemSet(), name, strict, report);
        }

        //!
        //! Search a service by name in any network of a given type of the file.
        //! @param [out] net Returned network of the service.
        //! @param [out] ts Returned transport stream of the service.
        //! @param [out] srv Returned service.
        //! @param [in] delsys Search only for these delivery systems. If empty, search any network.
        //! @param [in] name Service name.
        //! @param [in] strict If true, search exactly @a name.
        //! If false, the comparison is case-insensitive and spaces are ignored.
        //! If false, @a name can also be "major.minor" for ATSC services.
        //! @param [in,out] report Where to report errors.
        //! @return True if the service is found, false if the specified service does not exist.
        //!
        bool searchService(NetworkPtr& net,
                           TransportStreamPtr& ts,
                           ServicePtr& srv,
                           const DeliverySystemSet& delsys,
                           const UString& name,
                           bool strict = true,
                           Report& report = CERR) const;

        //!
        //! Get tuner parameters from a service name in any network of the file.
        //! @param [out] tune Returned modulation parameters.
        //! @param [in] name Service name.
        //! @param [in] strict If true, search exactly @a name.
        //! If false, the comparison is case-insensitive and spaces are ignored.
        //! If false, @a name can also be "major.minor" for ATSC services.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false if the specified service does not exist.
        //!
        bool serviceToTuning(ModulationArgs& tune, const UString& name, bool strict = true, Report& report = CERR) const
        {
            return serviceToTuning(tune, DeliverySystemSet(), name, strict, report);
        }

        //!
        //! Get tuner parameters from a service name in any network of a given type of the file.
        //! @param [out] tune Returned modulation parameters. Unmodified if the channel is not found.
        //! @param [in] delsys Search only for these delivery systems. If empty, search any network.
        //! @param [in] name Service name.
        //! @param [in] strict If true, search exactly @a name.
        //! If false, the comparison is case-insensitive and spaces are ignored.
        //! If false, @a name can also be "major.minor" for ATSC services.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false if the specified service does not exist.
        //!
        bool serviceToTuning(ModulationArgs& tune, const DeliverySystemSet& delsys, const UString& name, bool strict = true, Report& report = CERR) const;

    private:
        NetworkVector _networks {};    // List of networks in the configuration.
        xml::Tweaks   _xmlTweaks {};   // XML formatting and parsing tweaks.
        UString       _fileName {};    // Name of loaded file.

        // Parse an XML document and load the content into this object.
        bool parseDocument(const xml::Document& doc);

        // Generate an XML document from the content of this object.
        bool generateDocument(xml::Document& doc) const;

        // Convert modulation parameters to and from XML.
        bool fromXML(ModulationArgs& mod, const xml::Element* element, TunerType tunerType, uint16_t ts_id);
        xml::Element* toXML(const ModulationArgs& mod, xml::Element* parent) const;
    };
}
