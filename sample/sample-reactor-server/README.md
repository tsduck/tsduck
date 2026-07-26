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

### Sample usage

Start a clear server session:
~~~
$ ./reactor-server
Server listens on [::]:7777
~~~

Test client using curl. The option `-i` includes the response headers in the output.
~~~
$ curl http://localhost:7777/ -i
HTTP/1.0 204 No Content
Server: Same-Reactor-Server
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
Server: Same-Reactor-Server
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

Test client using curl. We specify `https` to use TLS. We also specify option `-k`
which means "accept insecure server certificate". This is required because our
server certificate is self-signed and cannot be verified from a valid Certification
Authority (CA).
~~~
$ curl https://localhost:7777/ -i -k
HTTP/1.0 204 No Content
Server: Same-Reactor-Server
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
