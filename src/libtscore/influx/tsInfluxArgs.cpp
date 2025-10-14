//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInfluxArgs.h"
#include "tsArgs.h"
#include "tsEnvironment.h"
#include "tsConfigFile.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::InfluxArgs::defineArgs(Args& args)
{
    args.option(u"active-config", 'c', Args::STRING);
    args.help(u"active-config",
              u"Config name to use in the InfluxDB CLI configurations file. "
              u"By default, use the environment variable INFLUX_ACTIVE_CONFIG, "
              u"then look for the active configuration in the configuration file, "
              u"or the first configuration if none is marked as active.");

    args.option(u"bucket", 'b', Args::STRING);
    args.help(u"bucket", u"name",
              u"Name of the InfluxDB bucket. "
              u"By default, use the environment variable INFLUX_BUCKET_NAME.");

    args.option(u"bucket-id", 0, Args::STRING);
    args.help(u"bucket-id", u"id",
              u"Identifier of the InfluxDB bucket. The 'id' must be a 16-character value. "
              u"By default, use the environment variable INFLUX_BUCKET_ID. "
              u"Only one of --bucket and --bucket-id shall be specified.");

    args.option(u"configs-path", 0, Args::FILENAME);
    args.help(u"configs-path",
              u"Path to the InfluxDB CLI configurations file. "
              u"By default, use the environment variable INFLUX_CONFIGS_PATH, then $HOME/.influxdbv2/configs.");

    args.option(u"host-url", 'h', Args::STRING);
    args.help(u"host-url", u"name",
              u"Host name or URL of the InfluxDB server. "
              u"If a host name is used instead of a URL, http: is assumed. "
              u"By default, use the environment variable INFLUX_HOST, then the InfluxDB CLI configuration file.");

    args.option(u"org", 'o', Args::STRING);
    args.help(u"org", u"name",
              u"Name of the InfluxDB organization. "
              u"By default, use the environment variable INFLUX_ORG, then the InfluxDB CLI configuration file.");

    args.option(u"org-id", 0, Args::STRING);
    args.help(u"org-id", u"id",
              u"Identifier of the InfluxDB organization. The 'id' must be a 16-character value. "
              u"By default, use the environment variable INFLUX_ORG_ID. "
              u"Only one of --org and --org-id shall be specified.");

    args.option(u"queue-size", 0, Args::POSITIVE);
    args.help(u"queue-size", u"count",
              u"Maximum number of queued metrics between the plugin thread and the communication thread with InfluxDB. "
              u"On off-line streams which are processed at high speed, increase this value if some metrics are lost. "
              u"The default queue size is " + UString::Decimal(DEFAULT_QUEUE_SIZE) + u" messages.");

    args.option(u"tag", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"tag", u"name=value",
              u"Add the specified tag, with the specified value, to all metrics which are sent to InfluxDB. "
              u"This can be used to identify a source of metrics and filter it using InfluxDB queries. "
              u"Several --tag options may be specified.");

    args.option(u"token", 't', Args::STRING);
    args.help(u"token", u"string",
              u"Token to authenticate InfluxDB requests. "
              u"By default, use the environment variable INFLUX_TOKEN, then the InfluxDB CLI configuration file.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::InfluxArgs::loadArgs(Args& args, bool required)
{
    bool success = true;

    // Get values from the command line.
    args.getPathValue(config_file, u"configs-path");
    args.getValue(config_name, u"active-config");
    args.getValue(host_url, u"host-url");
    args.getValue(org, u"org");
    args.getValue(org_id, u"org-id");
    args.getValue(bucket, u"bucket");
    args.getValue(bucket_id, u"bucket-id");
    args.getValue(token, u"token");
    args.getValues(additional_tags, u"tag");
    args.getIntValue(queue_size, u"queue-size", DEFAULT_QUEUE_SIZE);

    for (auto& tv : additional_tags) {
        if (!tv.contains(u'=')) {
            args.error(u"invalid --tag definition '%s', use name=value", tv);
            success = false;
        }
    }
    if (!org.empty() && !org_id.empty()) {
        args.error(u"only one of --org and --org-id shall be specified");
        success = false;
    }
    if (!bucket.empty() && !bucket_id.empty()) {
        args.error(u"only one of --bucket and --bucket-id shall be specified");
        success = false;
    }

    // Get defaults from environment variables.
    // Org and bucket ids take precedence over names.
    if (host_url.empty()) {
        host_url = GetEnvironment(u"INFLUX_HOST");
    }
    if (token.empty()) {
        token = GetEnvironment(u"INFLUX_TOKEN");
    }
    if (org_id.empty() && org.empty()) {
        org_id = GetEnvironment(u"INFLUX_ORG_ID");
    }
    if (org.empty() && org_id.empty()) {
        org = GetEnvironment(u"INFLUX_ORG");
    }
    if (bucket_id.empty() && bucket.empty()) {
        bucket_id = GetEnvironment(u"INFLUX_BUCKET_ID");
    }
    if (bucket.empty() && bucket_id.empty()) {
        bucket = GetEnvironment(u"INFLUX_BUCKET_NAME");
    }
    if (config_name.empty()) {
        config_name = GetEnvironment(u"INFLUX_ACTIVE_CONFIG");
    }
    if (config_file.empty()) {
        config_file = GetEnvironment(u"INFLUX_CONFIGS_PATH");
    }
    if (config_file.empty()) {
        config_file = UserHomeDirectory();
        config_file /= u".influxdbv2";
        config_file /= u"configs";
    }

    // Load the configuration file if something is missing.
    // The configuration file can only contain "url", "token", "org".
    if ((host_url.empty() || token.empty() || (org_id.empty() && org.empty())) && fs::exists(config_file)) {

        ConfigFile file(config_file, args);
        const ConfigSection* config = nullptr;

        if (!file.isLoaded()) {
            args.error(u"error loading InfluxDB config file: %s",  config_file);
            success = false;
        }
        else {
            // Find the active configuration.
            if (!config_name.empty()) {
                // A configuration name is specified.
                if (file.hasSection(config_name)) {
                    config = &file.section(config_name);
                }
                else if (required) {
                    args.error(u"configuration %s not found in %s", config_name, config_file);
                    success = false;
                }
            }
            else {
                // No configuration specified, find an active one, or first one by default.
                const ConfigSection* first = nullptr;
                UStringVector names;
                file.getSectionNames(names);
                for (const auto& name : names) {
                    const ConfigSection* sec = &file.section(name);
                    if (first == nullptr) {
                        first = sec;
                    }
                    if (sec->boolValue(u"active")) {
                        config = sec;
                        break;
                    }
                }
                // First configuration by default.
                if (config == nullptr) {
                    config = first;
                }
            }
        }

        // Look for missing information in configuration.
        if (config != nullptr) {
            if (host_url.empty()) {
                host_url = config->value(u"url").toUnquoted();
            }
            if (token.empty()) {
                token = config->value(u"token").toUnquoted();
            }
            if (org.empty() && org_id.empty()) {
                org = config->value(u"org").toUnquoted();
            }
        }
    }

    // The host_url can be a URL or a host name. Make sure it is an URL.
    if (!host_url.empty()) {
        if (!host_url.contains(u"://")) {
            host_url.insert(0, u"http://");
        }
        if (!host_url.ends_with(u'/')) {
            host_url.append(u'/');
        }
    }

    // If connection to InfluxDB is required, make sure we have everything we need.
    if (required) {
        if (host_url.empty()) {
            args.error(u"missing InfluxDB host, use --host-url");
            success = false;
        }
        if (token.empty()) {
            args.error(u"missing InfluxDB token, use --token");
            success = false;
        }
        if (org.empty() && org_id.empty()) {
            args.error(u"missing InfluxDB organization, use --org or --org-id");
            success = false;
        }
        if (bucket.empty() && bucket_id.empty()) {
            args.error(u"missing InfluxDB bucket, use --bucket or --bucket-id");
            success = false;
        }
    }

    args.debug(u"InfluxDB host: %s", host_url);
    args.debug(u"InfluxDB token: %s", token);
    args.debug(u"InfluxDB org: %s", org);
    args.debug(u"InfluxDB org id: %s", org_id);
    args.debug(u"InfluxDB bucket: %s", bucket);
    args.debug(u"InfluxDB bucket id: %s", bucket_id);
    args.debug(u"InfluxDB configuration file: %s", config_file);
    args.debug(u"InfluxDB configuration name: %s", config_name);

    return success;
}
