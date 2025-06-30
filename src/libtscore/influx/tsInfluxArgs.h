//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for InfluxDB connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    class Args;

    //!
    //! Command line arguments for InfluxDB connection.
    //! @ingroup libtscore net
    //!
    //! Same options and resolution of defaults as InfluxDB CLI.
    //! @see https://docs.influxdata.com/influxdb/v2/reference/cli/influx/
    //!
    class TSCOREDLL InfluxArgs
    {
    public:
        //!
        //! Constructor.
        //!
        InfluxArgs() = default;

        // Public fields, by options.
        UString  host_url {};      //!< -\-host-url (-h) [INFLUX_HOST], URL or host name
        UString  token {};         //!< -\-token (-t) [INFLUX_TOKEN]
        UString  org {};           //!< -\-org (-o) [INFLUX_ORG]
        UString  org_id {};        //!< -\-org-id [INFLUX_ORG_ID]
        UString  bucket {};        //!< -\-bucket (-b) [INFLUX_BUCKET_NAME]
        UString  bucket_id {};     //!< -\-bucket-id [INFLUX_BUCKET_ID]
        UString  config_name {};   //!< -\-active-config (-c) [INFLUX_ACTIVE_CONFIG]
        fs::path config_file {};   //!< -\-configs-path [INFLUX_CONFIGS_PATH]

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Apply defaultss from environment variables and Influx configurations file.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] required If true, all arguments to connect to InfluuxDB are required.
        //! When false, the arguments are optional.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args, bool required);
    };
}
