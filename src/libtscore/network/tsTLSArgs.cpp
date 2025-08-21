//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSArgs.h"
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSArgs::TLSArgs(const UString& description, const UString& prefix) :
    SuperClass(description, prefix),
    _opt_tls(_prefix + u"tls"),
    _opt_insecure(_prefix + u"insecure"),
    _opt_certificate_store(_prefix + u"store"),
    _opt_certificate_path(_prefix + u"certificate-path"),
    _opt_key_path(_prefix + u"key-path")
{
}

ts::TLSArgs::~TLSArgs()
{
}


//----------------------------------------------------------------------------
// Command line options for a TLS server.
//----------------------------------------------------------------------------

void ts::TLSArgs::defineServerArgs(Args& args)
{
    SuperClass::defineServerArgs(args);

    args.option(_opt_tls.c_str());
    args.help(_opt_tls.c_str(),
              u"The " + _description + " uses SSL/TLS. "
              u"In that case, a server certificate is required. "
              u"By default, use unencrypted communications.");

    args.option(_opt_certificate_path.c_str(), 0, Args::STRING);
    args.help(_opt_certificate_path.c_str(), u"name",
              u"With --" + _opt_tls + u", path to the certificate for the " + _description + u". "
              u"The default value is the value of environment variable TSDUCK_TLS_CERTIFICATE.\n"
              u"On UNIX systems, this is the path name of the certificate file in PEM format.\n"
              u"On Windows, this is the name of a certificate in the user or system store.");

    args.option(_opt_key_path.c_str(), 0, Args::STRING);
    args.help(_opt_key_path.c_str(), u"name",
              u"With --" + _opt_tls + u", path to the private key for the " + _description + u". "
              u"The default value is the value of environment variable TSDUCK_TLS_KEY.\n"
              u"On UNIX systems, this is the path name of the private key file in PEM format.\n"
              u"On Windows, the private key is retrieved with the certificate and this parameter is unused.");

    args.option(_opt_certificate_store.c_str(), 0, Args::STRING);
    args.help(_opt_certificate_store.c_str(), u"name",
              u"With --" + _opt_tls + u", path to the certificate store for the " + _description + u". "
              u"The default value is the value of environment variable TSDUCK_TLS_STORE.\n"
              u"On Windows, the possible values are \"system\" (Cert:\\LocalMachine\\My) "
              u"and \"user\" (Cert:\\CurrentUser\\My). The default is \"user\".\n"
              u"On UNIX systems, this parameter is unused.");
}


//----------------------------------------------------------------------------
// Load command line options for a TLS server.
//----------------------------------------------------------------------------

bool ts::TLSArgs::loadServerArgs(Args& args, const UChar* server_option)
{
    static constexpr const UChar* default_store =
#if defined(TS_WINDOWS)
        u"user";
#else
        u"";
#endif

    use_tls = args.present(_opt_tls.c_str());
    args.getValue(certificate_path, _opt_certificate_path.c_str(), GetEnvironment(u"TSDUCK_TLS_CERTIFICATE").c_str());
    args.getValue(key_path, _opt_key_path.c_str(), GetEnvironment(u"TSDUCK_TLS_KEY").c_str());
    args.getValue(certificate_store, _opt_certificate_store.c_str(), GetEnvironment(u"TSDUCK_TLS_STORE", default_store).c_str());
    return SuperClass::loadServerArgs(args, server_option);
}


//----------------------------------------------------------------------------
// Command line options for a TLS client.
//----------------------------------------------------------------------------

void ts::TLSArgs::defineClientArgs(Args& args)
{
    SuperClass::defineClientArgs(args);

    args.option(_opt_tls.c_str());
    args.help(_opt_tls.c_str(),
              u"Connect to the " + _description + " using SSL/TLS. "
              u"By default, use unencrypted communications.");

    args.option(_opt_insecure.c_str());
    args.help(_opt_insecure.c_str(),
              u"With --" + _opt_tls + u", do not verify the TLS server's certificate. "
              u"Use with care because it opens the door the man-in-the-middle attacks.");
}


//----------------------------------------------------------------------------
// Load command line options for a TLS client.
//----------------------------------------------------------------------------

bool ts::TLSArgs::loadClientArgs(Args& args, const UChar* server_option)
{
    use_tls = args.present(_opt_tls.c_str());
    insecure = args.present(_opt_insecure.c_str());
    return SuperClass::loadClientArgs(args, server_option);
}
