## Sample server using the reactor pattern

This directory contains a sample third-party application which is not part
of TSDuck but uses the TSDuck library.

### Prerequisites

To be able to build this sample application, you must install the TSDuck
development environment first. On Windows systems, you must select the
optional "Development" component during the installation. On Ubuntu systems,
you must install the package tsduck-dev. On Fedora, Red Hat and CentOS, you
must install the package tsduck-devel.

### Building the application on Linux

Just run "make". The makefile in this directory contains various calls to
the 'tsconfig' utility which generates the various required compilation
and link options.

### Building the application on Windows

Open the solution file "reactor-server.sln" using Visual Studio and build
the application. The project file "reactor-server.vcxproj" contains the
following additional line, just before the final line:

  <Import Project="$(TSDUCK)\tscore.props" />

The included property file sets all required options to use the TSDuck library.

### Command line syntax

The sample program uses the TSDuck command line arguments features, including
predefined options for TLS servers. Therefore, it displays its usage using the
option `--help`.
~~~
$ ./reactor-server --help

Sample line-based TLS server using the reactor pattern

Usage: reactor-server [options] [local-address:]port

Parameters:

  TCP port number of the server. The default port is 7777. If an optional
  address is specified, it must be a local interface and the server listens to
  that interface only.

Options:

  --certificate-path name
      With --tls, path to the certificate for the server. The default value is
      the value of environment variable TSDUCK_TLS_CERTIFICATE.
      On UNIX systems, this is the path name of the certificate file in PEM
      format.
      On Windows, this is the name of a certificate in the user or system
      store.

  -d[level]
  --debug[=level]
      Produce debug traces. The default level is 1. Higher levels produce more
      messages.

  --ephemeral-rsa-bits value
      With --tls, create an ephemeral self-signed certificate for the server.
      The value specifies the size in bits of the ephemeral RSA key which it
      generated. The default value is the value of environment variable
      TSDUCK_TLS_EPHEMERAL_RSA_BITS.
      Keep in mind that ephemeral self-signed certificates are considered as
      "invalid" or "insecure" by client applications. Be sure to disable the
      verification of the TLS server's certificate on the client side. By
      default, the server needs a designated persistent certificate.

  --help
      Display this help text.

  --key-path name
      With --tls, path to the private key for the server. The default value is
      the value of environment variable TSDUCK_TLS_KEY.
      On UNIX systems, this is the path name of the private key file in PEM
      format.
      On Windows, the private key is retrieved with the certificate and this
      parameter is unused.

  --store name
      With --tls, path to the certificate store for the server. The default
      value is the value of environment variable TSDUCK_TLS_STORE.
      On Windows, the possible values are "system" (Cert:\LocalMachine\My) and
      "user" (Cert:\CurrentUser\My). The default is "user".
      On UNIX systems, this parameter is unused.

  --tls
      The server uses SSL/TLS. In that case, a server certificate is required.
      By default, use unencrypted communications.

  -v
  --verbose
      Produce verbose output.

  --version[=name]
      Display the TSDuck version number.
      The 'name' must be one of "acceleration", "all", "compiler", "crypto",
      "date", "http", "integer", "long", "short", "system", "tls", "zlib".
~~~

### Sample usages

Start a clear (unencrypted) TCP server session:
~~~
$ ./reactor-server
Server listens on [::]:7777
~~~

Test client using curl: The option `-i` includes the response headers in the output.
~~~
$ curl http://localhost:7777/ -i
HTTP/1.0 204 No Content
Server: Sample-Reactor-Server
Connection: close
X-Session-Name: [::1]:52345-[::1]:7777
X-Echo-Request: GET / HTTP/1.1
X-Echo-Header: Host: localhost:7777
X-Echo-Header: User-Agent: curl/8.7.1
X-Echo-Header: Accept: */*
~~~

To exit the server, send a request with header "X-Exit".
~~~
$ curl http://localhost:7777/ -i -H "X-Exit: true"
HTTP/1.0 204 No Content
Server: Sample-Reactor-Server
Connection: close
X-Session-Name: [::1]:53162-[::1]:7777
X-Echo-Request: GET / HTTP/1.1
X-Echo-Header: Host: localhost:7777
X-Echo-Header: User-Agent: curl/8.7.1
X-Echo-Header: Accept: */*
X-Echo-Header: X-Exit: true
X-Message: exiting server
~~~

Start a server session using TLS encryption:
~~~
$ ./reactor-server --tls --ephemeral-rsa-bits 2048
Server listens on [::]:7777
~~~

Because we don't have a valid server certificate in this test, we generate a
self-signed certificate on the fly using a 2048-bit ephemeral RSA key pair.

Test client using curl: We specify `https:` to use TLS. We also specify option `-k`
which means "accept insecure server certificate". This is required because our
server certificate is self-signed and cannot be verified from a valid Certification
Authority (CA).
~~~
$ curl https://localhost:7777/ -i -k
HTTP/1.0 204 No Content
Server: Sample-Reactor-Server
Connection: close
X-Session-Name: [::1]:53182-[::1]:7777
X-Echo-Request: GET / HTTP/1.1
X-Echo-Header: Host: localhost:7777
X-Echo-Header: User-Agent: curl/8.7.1
X-Echo-Header: Accept: */*
~~~

Without option `-k` (insecure), we would get this:
~~~
$ curl https://localhost:7777/ -i
curl: (60) SSL certificate problem: unable to get local issuer certificate
More details here: https://curl.se/docs/sslcerts.html

curl failed to verify the legitimacy of the server and therefore could not
establish a secure connection to it. To learn more about this situation and
how to fix it, please visit the web page mentioned above.
~~~
