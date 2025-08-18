//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRestArgs.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::RestArgs::RestArgs(const UString& description, const UString& prefix) :
    _description(description),
    _prefix(prefix + (prefix.empty() || prefix.ends_with('-') ? u"" : u"-")),
    _opt_tls(_prefix + u"tls"),
    _opt_insecure(_prefix + u"insecure"),
    _opt_token(_prefix + u"token"),
    _opt_certificate_store(_prefix + u"store"),
    _opt_certificate_path(_prefix + u"certificate-path"),
    _opt_key_path(_prefix + u"key-path")
{
}


//----------------------------------------------------------------------------
// Command line options for a TLS server.
//----------------------------------------------------------------------------

void ts::RestArgs::defineServerArgs(Args& args, bool with_auth)
{
    args.option(_opt_tls.c_str());
    args.help(_opt_tls.c_str(),
              u"The " + _description + " uses SSL/TLS. "
              u"In that case, a server certificate is required. "
              u"By default, use unencrypted communications.");

    if (with_auth) {
        args.option(_opt_token.c_str(), 0, Args::STRING);
        args.help(_opt_token.c_str(), u"string",
                  u"Optional authentication token that clients are required to provide. "
                  u"For security reasons, use only with --" + _opt_tls + u".");
    }

    args.option(_opt_certificate_path.c_str(), 0, Args::STRING);
    args.help(_opt_certificate_path.c_str(), u"name",
              u"With --" + _opt_tls + u", path to the certificate for the " + _description + ".\n"
              u"On UNIX systems, this is the path name of the certificate file in PEM format.\n"
              u"On Windows, this is the name of a certificate in the user or system store.");

    args.option(_opt_key_path.c_str(), 0, Args::STRING);
    args.help(_opt_key_path.c_str(), u"name",
              u"With --" + _opt_tls + u", path to the private key for the " + _description + ".\n"
              u"On UNIX systems, this is the path name of the private key file in PEM format.\n"
              u"On Windows, the private key is retrieved with the certificate and this parameter is unused.");

    args.option(_opt_certificate_store.c_str(), 0, Args::STRING);
    args.help(_opt_certificate_store.c_str(), u"name",
              u"With --" + _opt_tls + u", path to the certificate store for the " + _description + ".\n"
              u"On Windows, the possible values are \"system\" (Cert:\\LocalMachine\\My) "
              u"or \"user\" (Cert:\\CurrentUser\\My). The default is \"user\".\n"
              u"On UNIX systems, this parameter is unused.");
}

bool ts::RestArgs::loadServerArgs(Args& args, bool with_auth)
{
    use_tls = args.present(_opt_tls.c_str());
    args.getValue(certificate_path, _opt_certificate_path.c_str());
    args.getValue(key_path, _opt_key_path.c_str());
    args.getValue(certificate_store, _opt_certificate_store.c_str());
    if (with_auth) {
        args.getValue(auth_token, _opt_token.c_str());
    }
    return true;
}


//----------------------------------------------------------------------------
// Command line options for a TLS client.
//----------------------------------------------------------------------------

void ts::RestArgs::defineClientArgs(Args& args, bool with_auth)
{
    args.option(_opt_tls.c_str());
    args.help(_opt_tls.c_str(),
              u"Connect to the " + _description + " using SSL/TLS. "
              u"By default, use unencrypted communications.");

    args.option(_opt_insecure.c_str());
    args.help(_opt_insecure.c_str(),
              u"With --" + _opt_tls + u", do not verify the TLS server's certificate. "
              u"Use with care because it opens the door the man-in-the-middle attacks.");

    if (with_auth) {
        args.option(_opt_token.c_str(), 0, Args::STRING);
        args.help(_opt_token.c_str(), u"string",
                  u"Authentication token for the server, if required.");
    }
}

bool ts::RestArgs::loadClientArgs(Args& args, bool with_auth)
{
    use_tls = args.present(_opt_tls.c_str());
    insecure = args.present(_opt_insecure.c_str());
    if (with_auth) {
        args.getValue(auth_token, _opt_token.c_str());
    }
    return true;
}
