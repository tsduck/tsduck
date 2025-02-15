//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRISTPluginData.h"
#include "tsMemory.h"

#if defined(TS_NO_RIST)
TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsRISTPluginDataIsEmpty = true; // Avoid warning about empty module.
#else

//----------------------------------------------------------------------------
// Input/output common data constructor.
//----------------------------------------------------------------------------

ts::RISTPluginData::RISTPluginData(Report& report) :
    _report(report)
{
    // We need to initialize the log structure in two steps:
    // 1) set void data, 2) set the real data using rist_logging_set().
    // If we directly initialize the log fields without calling rist_logging_set(),
    // the global internal mutex of the RIST logging system is not initialized
    // and some logging features will crash later, trying to lock an uninitialized mutex.

    log.log_level = RIST_LOG_DISABLE;
    log.log_cb = nullptr;
    log.log_cb_arg = nullptr;
    log.log_socket = -1;
    log.log_stream = nullptr;

    ::rist_logging_settings* pset = &log;
    ::rist_logging_set(&pset, SeverityToRistLog(_report.maxSeverity()), LogCallback, this, nullptr, nullptr);
}


//----------------------------------------------------------------------------
// Add command line option definitions in an Args.
//----------------------------------------------------------------------------

void ts::RISTPluginData::defineArgs(Args& args)
{
    args.option(u"", 0, Args::STRING, 1, Args::UNLIMITED_COUNT);
    args.help(u"",
              u"One or more RIST URL's. "
              u"A RIST URL (rist://...) may include tuning parameters in addition to the address and port. "
              u"See https://code.videolan.org/rist/librist/-/wikis/LibRIST%20Documentation for more details.");

    args.option(u"allow", 'a', Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"allow", u"ip-address[:port]",
              u"In listener mode (rist://@...), allow the specified IP address (and optional port) to connect. "
              u"More than one --allow option can be used to specify several allowed addresses. "
              u"If at least one --allow option is specified, any client which is not explicitly allowed is denied.");

    args.option(u"deny", 'd', Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"deny", u"ip-address[:port]",
              u"In listener mode (rist://@...), deny the specified IP address (and optional port) to connect. "
              u"More than one --deny option can be used to specify several denied addresses.");

    args.option(u"buffer-size", 'b', Args::POSITIVE);
    args.help(u"buffer-size", u"milliseconds",
              u"Default buffer size in milliseconds for packet retransmissions. "
              u"This value overrides the 'buffer=' parameter in the URL.");

    args.option(u"encryption-type", 0, Names({ // actual value is an AES key size in bits
        {u"AES-128", 128},
        {u"AES-256", 256},
    }));
    args.help(u"encryption-type", u"name",
              u"Specify the encryption type (none by default). "
              u"This value is used when the 'aes-type=' parameter is not present in the URL.");

    args.option(u"profile", 0, Names({
        {u"simple",   RIST_PROFILE_SIMPLE},
        {u"main",     RIST_PROFILE_MAIN},
        {u"advanced", RIST_PROFILE_ADVANCED},
    }));
    args.help(u"profile", u"name", u"Specify the RIST profile (main profile by default).");

    args.option(u"secret", 's', Args::STRING);
    args.help(u"secret", u"string",
              u"Default pre-shared encryption secret. "
              u"If a pre-shared secret is specified without --encryption-type, AES-128 is used by default. "
              u"This value is used when the 'secret=' parameter is not present in the URL.");

    args.option(u"stats-interval", 0, Args::POSITIVE);
    args.help(u"stats-interval", u"milliseconds",
              u"Periodically report a line of statistics. The interval is in milliseconds. "
              u"The statistics are in JSON format.");

    args.option(u"stats-prefix", 0, Args::STRING);
    args.help(u"stats-prefix", u"'prefix'",
              u"With --stats-interval, specify a prefix to prepend on the statistics line "
              u"before the JSON text to locate the appropriate line in the logs.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::RISTPluginData::loadArgs(DuckContext& duck, Args& args)
{
    // Make sure we do not have any allocated resources from librist.
    cleanup();

    // Common rist plugin options.
    args.getValues(_peer_urls, u"");
    args.getIntValue(profile, u"profile", RIST_PROFILE_MAIN);
    args.getIntValue(_buffer_size, u"buffer-size");
    args.getIntValue(_encryption_type, u"encryption-type", 0);
    args.getValue(_secret, u"secret");
    args.getIntValue(_stats_interval, u"stats-interval", 0);
    args.getValue(_stats_prefix, u"stats-prefix");

    // Client address filter lists.
    if (!getSocketValues(args, _allowed, u"allow") || !getSocketValues(args, _denied, u"deny")) {
        return false;
    }

    // Get the UTF-8 version of the pre-shared secret.
    const std::string secret8(_secret.toUTF8());

    // Parse all URL's. The rist_peer_config are allocated by the library.
    _peer_configs.resize(_peer_urls.size());
    for (size_t i = 0; i < _peer_urls.size(); ++i) {

        // Parse the URL.
        _peer_configs[i] = nullptr;
        if (::rist_parse_address2(_peer_urls[i].toUTF8().c_str(), &_peer_configs[i]) != 0 || _peer_configs[i] == nullptr) {
            _report.error(u"invalid RIST URL: %s", _peer_urls[i]);
            cleanup();
            return false;
        }

        // Override URL parameters with command-line options.
        ::rist_peer_config* const peer = _peer_configs[i];
        if (_buffer_size > 0) {
            // Unconditionally override 'buffer='
            peer->recovery_length_max = peer->recovery_length_min = _buffer_size;
        }
        if (!_secret.empty() && peer->secret[0] == '\0') {
            // Override 'secret=' only if not specified in the URL.
            if (secret8.size() >= sizeof(peer->secret)) {
                _report.error(u"invalid shared secret, maximum length is %d characters", sizeof(peer->secret) - 1);
                return false;
            }
            TS_ZERO(peer->secret);
            MemCopy(peer->secret, secret8.data(), secret8.size());
        }
        if (peer->secret[0] != '\0' && peer->key_size == 0) {
            // Override 'aes-type=' if unspecified and a secret is specified (AES-128 by default).
            peer->key_size = _encryption_type == 0 ? 128 : _encryption_type;
        }
        if (peer->secret[0] == '\0' && peer->key_size != 0) {
            _report.error(u"AES-%d encryption is specified but the shared secret is missing", peer->key_size);
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Input/output common data cleanup.
//----------------------------------------------------------------------------

void ts::RISTPluginData::cleanup()
{
    // Deallocate all peer configurations (parsed RIST URL's).
    for (size_t i = 0; i < _peer_configs.size(); ++i) {
        if (_peer_configs[i] != nullptr) {
            ::rist_peer_config_free2(&_peer_configs[i]);
            _peer_configs[i] = nullptr;
        }
    }
    _peer_configs.clear();

    // Close the RIST context.
    if (ctx != nullptr) {
        ::rist_destroy(ctx);
        ctx = nullptr;
    }
}


//----------------------------------------------------------------------------
// Analyze a list of options containing socket addresses.
//----------------------------------------------------------------------------

bool ts::RISTPluginData::getSocketValues(Args& args, IPSocketAddressVector& list, const UChar* option)
{
    const size_t count = args.count(option);
    list.resize(count);
    for (size_t index = 0; index < count; ++index) {
        const UString str(args.value(option, u"", index));
        if (!list[index].resolve(str, _report) || !list[index].hasAddress()) {
            _report.error(u"invalid socket address \"%s\", use \"address[:port]\"", str);
            return false;
        }
    }
    return true; // success
}


//----------------------------------------------------------------------------
// Add all URL's as peers in the RIST context.
//----------------------------------------------------------------------------

bool ts::RISTPluginData::addPeers()
{
    // Setup statistics callback if required.
    if (_stats_interval > 0 && ::rist_stats_callback_set(ctx, _stats_interval, StatsCallback, this) < 0) {
        _report.warning(u"error setting statistics callback");
    }

    // Setup connection callback.
    if (::rist_auth_handler_set(ctx, ConnectCallback, DisconnectCallback, this)) {
        _report.warning(u"error setting connection callback");
    }

    // Add peers one by one.
    for (size_t i = 0; i < _peer_configs.size(); ++i) {

        // Create the peer.
        ::rist_peer* peer = nullptr;
        ::rist_peer_config* config = _peer_configs[i];
        if (::rist_peer_create(ctx, &peer, config) != 0) {
            _report.error(u"error creating peer: %s", _peer_urls[i]);
            cleanup();
            return false;
        }

        // Add user authentication if specified in URL.
        if (config->srp_username[0] != '\0' && config->srp_password[0] != '\0') {

            // The API changed in librist 0.2.8 (API version 4.3.0).
            #if LIBRIST_API_VERSION < LIBRIST_MAKE_API_VERSION(4, 3, 0)
                const int err = ::rist_enable_eap_srp(peer, config->srp_username, config->srp_password, nullptr, nullptr);
            #else
                const int err = ::rist_enable_eap_srp_2(peer, config->srp_username, config->srp_password, nullptr, nullptr);
            #endif

            if (err != 0) {
                // Report warning but do not fail.
                _report.warning(u"error %d while setting SRP authentication on %s", err, _peer_urls[i]);
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// RIST connection/disconnection callbacks using a RISTPluginData* argument.
//----------------------------------------------------------------------------

int ts::RISTPluginData::ConnectCallback(void* arg, const char* peer_ip, uint16_t peer_port, const char* local_ip, uint16_t local_port, ::rist_peer* peer)
{
    RISTPluginData* data = reinterpret_cast<RISTPluginData*>(arg);
    if (data == nullptr || peer_ip == nullptr || local_ip == nullptr) {
        // Looks like an invalid call, reject connection just in case of hacking.
        return -1;
    }
    data->_report.verbose(u"connected to %s:%d (local: %s:%d)", peer_ip, peer_port, local_ip, local_port);

    // Process client access filtering if necessary.
    if (!data->_allowed.empty() || !data->_denied.empty()) {

        // Analyze remote peer socket address.
        IPSocketAddress addr;
        if (!addr.resolve(UString::FromUTF8(peer_ip), data->_report)) {
            data->_report.error(u"invalid peer address: %s", peer_ip);
            return -1; // connection rejected
        }
        addr.setPort(peer_port);

        // Process black list first.
        for (auto it = data->_denied.begin(); it != data->_denied.end(); ++it) {
            if (it->match(addr)) {
                data->_report.error(u"peer address %s is denied, connection rejected", addr);
                return -1; // connection rejected
            }
        }

        // Then process white list if not empty.
        bool ok = data->_allowed.empty();
        for (auto it = data->_allowed.begin(); !ok && it != data->_allowed.end(); ++it) {
            ok = it->match(addr);
        }
        if (!ok) {
            data->_report.error(u"peer address %s is not explicitly allowed, connection rejected", addr);
            return -1; // connection rejected
        }
    }
    return 0; // connection accepted
}

int ts::RISTPluginData::DisconnectCallback(void* arg, ::rist_peer* peer)
{
    // We do not do anything here. According to the RIST docs, it should be possible
    // to set a non-null connect callback with a null disconnect callback. However,
    // the application crashes on disconnection. We must specify both callbacks or
    // none. So, we have an empty one here.
    return 0;
}


//----------------------------------------------------------------------------
// Convert between RIST log level and TSDuck severity.
//----------------------------------------------------------------------------

int ts::RISTPluginData::RistLogToSeverity(::rist_log_level level)
{
    switch (level) {
        case RIST_LOG_ERROR:
            return ts::Severity::Error;
        case RIST_LOG_WARN:
            return ts::Severity::Warning;
        case RIST_LOG_NOTICE:
            return ts::Severity::Info;
        case RIST_LOG_INFO:
            return ts::Severity::Verbose;
        case RIST_LOG_DEBUG:
            return ts::Severity::Debug;
        case RIST_LOG_SIMULATE:
            return 2; // debug level 2.
        case RIST_LOG_DISABLE:
        default:
            return 100; // Probably never activated
    }
}

::rist_log_level ts::RISTPluginData::SeverityToRistLog(int severity)
{
    switch (severity) {
        case ts::Severity::Fatal:
        case ts::Severity::Severe:
        case ts::Severity::Error:
            return RIST_LOG_ERROR;
        case ts::Severity::Warning:
            return RIST_LOG_WARN;
        case ts::Severity::Info:
            return RIST_LOG_NOTICE;
        case ts::Severity::Verbose:
            return RIST_LOG_INFO;
        case ts::Severity::Debug:
            return RIST_LOG_DEBUG;
        default:
            return RIST_LOG_DISABLE;
    }
}


//----------------------------------------------------------------------------
// A RIST log callback using a RISTPluginData* argument.
//----------------------------------------------------------------------------

int ts::RISTPluginData::LogCallback(void* arg, ::rist_log_level level, const char* msg)
{
    RISTPluginData* data = reinterpret_cast<RISTPluginData*>(arg);
    if (data != nullptr && msg != nullptr) {
        UString line;
        line.assignFromUTF8(msg);
        while (!line.empty() && IsSpace(line.back())) {
            line.pop_back();
        }
        data->_report.log(RistLogToSeverity(level), line);
    }
    return 0; // undocumented, 0 seems safe
}


//----------------------------------------------------------------------------
// A RIST stats callback using a RISTPluginData* argument.
//----------------------------------------------------------------------------

int ts::RISTPluginData::StatsCallback(void* arg, const ::rist_stats* stats)
{
    RISTPluginData* data = reinterpret_cast<RISTPluginData*>(arg);
    if (data != nullptr && stats != nullptr) {
        data->_report.info(u"%s%s", data->_stats_prefix, stats->stats_json);
        ::rist_stats_free(stats);
    }
    return 0; // undocumented, 0 seems safe
}

#endif // TS_NO_RIST
