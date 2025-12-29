//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service in DVB-I and DVB-NIP.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteSessionId.h"

namespace ts::mcast {
    //!
    //! Representation of a service in DVB-I and DVB-NIP.
    //! Caution: This implementation is partial. Some part of the XML document are not deserialized.
    //! @see ETSI TS 103 770, section 5.5.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPService
    {
    public:
        //!
        //! Default constructor.
        //!
        NIPService() = default;

        //!
        //! Clear the content of the structure.
        //!
        void clear();

        //!
        //! Description of an instance of service.
        //! A service can be present on several media.
        //! The media file name is not present here, it is the index in the instance map.
        //! The media file is typically a HLS playlist, a MPEG-DASH manifest, etc.
        //!
        class TSDUCKDLL Instance
        {
        public:
            Instance() = default;                  //!< Constructor.
            uint32_t       instance_priority = 0;  //!< Priority of this instance.
            UString        media_type {};          //!< MIME type of the media for this instance (HLS playlist, DASH manifest, etc).
            FluteSessionId session_id {};          //!< Session where the service media are received.
        };

        // NIPService public fields.
        uint32_t channel_number = 0;               //!< Logical channel number (LCN).
        bool     selectable = true;                //!< Service is selectable.
        bool     visible = true;                   //!< Service is visible.
        UString  service_name {};                  //!< Service name.
        UString  service_type {};                  //!< Service type.
        UString  provider_name {};                 //!< Service provider name.
        std::map<UString, Instance> instances {};  //!< Map of service instances, indexed by media file name.
    };
}
