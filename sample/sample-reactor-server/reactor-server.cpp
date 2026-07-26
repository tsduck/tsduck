//----------------------------------------------------------------------------
//
// TSDuck sample line-based TLS server using the reactor pattern.
//
// This application behaves as a simple HTTP server. All headers are echoed.
// If a client header is X-Status, the server returns the number of clients.
// If a client header is X-Exit, the server exits.
//
//----------------------------------------------------------------------------

#include "tscore.h"

// Enforce COM and network init on Windows, transparent elsewhere.
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options.
//----------------------------------------------------------------------------

class CommandOptions: public ts::Args
{
    TS_NOBUILD_NOCOPY(CommandOptions);
public:
    // Command line default values.
    static constexpr uint16_t DEFAULT_SERVER_PORT = 7777;

    // Command line values.
    ts::IPSocketAddress server_address {};
    ts::TLSArgs tls_args {};

    // Constructor and destructor.
    CommandOptions(int argc, char* argv[]);
    virtual ~CommandOptions() override;
};

// Constructor.
CommandOptions::CommandOptions(int argc, char *argv[]) :
    Args(u"Sample line-based TLS server using the reactor pattern", u"[options] [local-address:]port")
{
    // Define generic command line options for TLS server.
    tls_args.defineServerArgs(*this);

    // Optional positional parameter.
    option(u"", 0, IPSOCKADDR_OA);
    help(u"",
         u"TCP port number of the server. "
         u"The default port is " + ts::UString::Decimal(DEFAULT_SERVER_PORT, 0, true, u"") + u". "
         u"If an optional address is specified, it must be a local interface and the server listens to that interface only.");

    // Analyze command.
    analyze(argc, argv);

    // Get command line options.
    tls_args.loadServerArgs(*this);
    getSocketValue(server_address, u"", ts::IPSocketAddress(ts::IPAddress::AnyAddress6, DEFAULT_SERVER_PORT));

    // Abort on error in the command line.
    exitOnError();
}

// Destructor.
CommandOptions::~CommandOptions()
{
}


//----------------------------------------------------------------------------
// This class manages a client connection in the server.
//----------------------------------------------------------------------------

class ClientConnection:
    public ts::ReactiveServerSessionInterface,
    private ts::ReactiveTCPConnectionHandlerInterface,
    private ts::ReactiveTextStreamHandlerInterface
{
    TS_NOBUILD_NOCOPY(ClientConnection);
public:
    // Constructor.
    ClientConnection(ts::Reactor& reactor, ts::ReactiveServer& server, CommandOptions& opt);

    // Implementation of ReactiveServerSessionInterface.
    virtual ts::ReactiveTCPConnection& getConnection() override;

private:
    // References to global objects.
    ts::Reactor& _reactor;
    ts::ReactiveServer& _server;
    CommandOptions& _opt;

    // Define all client elements.
    ts::TCPConnection _tcp_client {&_reactor};
    std::unique_ptr<ts::ReactiveTCPConnection> _react_client = _opt.tls_args.use_tls ?
        std::make_unique<ts::ReactiveTLSConnection>(_reactor, _tcp_client, _opt.tls_args) :
        std::make_unique<ts::ReactiveTCPConnection>(_reactor, _tcp_client);
    ts::ReactiveTextStream _text_client {*_react_client};

    // State of the HTTP request.
    enum State {START, HEADERS, CONTENT, END};

    // Working data.
    ts::UString _session_name {};
    State _state = START;

    // Implementation of ReactiveTCPConnectionHandlerInterface.
    virtual void handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data) override;
    virtual void handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data) override;

    // Implementation of ReactiveTCPConnectionHandlerInterface and ReactiveStreamHandlerInterface.
    virtual void handleWriteStream(ts::ReactiveStream& stream, int error_code, const ts::ObjectPtr& user_data) override;

    // Implementation of ReactiveTextStreamHandlerInterface.
    virtual void handleTextLine(ts::ReactiveTextStream& stream, const ts::UString& line, int error_code, const ts::ObjectPtr& user_data) override;
};

// Constructor.
ClientConnection::ClientConnection(ts::Reactor& reactor, ts::ReactiveServer& server, CommandOptions& opt) :
    _reactor(reactor),
    _server(server),
    _opt(opt)
{
    // Call us when a client session is accepted.
    _react_client->whenAccepted(this);
}

// Implementation of ReactiveServerSessionInterface.
// Called by the ReactiveServer to access our reactive connection.
ts::ReactiveTCPConnection& ClientConnection::getConnection()
{
    return *_react_client;
}

// Implementation of ReactiveTCPConnectionHandlerInterface.
// Called when a client connects.
void ClientConnection::handleTCPAccepted(ts::ReactiveTCPServer& server, ts::ReactiveTCPConnection& sock, int error_code, const ts::ObjectPtr& user_data)
{
    // When handleTCPAccepted() is called with an error, this is a way to notify that
    // the object won't be used for a connection, usually due to the server exiting.
    if (ts::SysSuccess(error_code)) {
        // Without error, this is a real new connection.
        _state = START;
        _session_name = sock.socket().peerName() + u"-" + sock.socket().localName();
        _opt.info(u"New session %s", _session_name);
        // Call us when a new text line is received.
        _text_client.startReadText(this);
    }
}

// Implementation of ReactiveTCPConnectionHandlerInterface.
// Called when the client disconnect and the connection socket is closed.
void ClientConnection::handleTCPClosed(ts::ReactiveTCPConnection& sock, const ts::ObjectPtr& user_data)
{
    _opt.info(u"Session %s closed", _session_name);
}

// Implementation of ReactiveTCPConnectionHandlerInterface and ReactiveStreamHandlerInterface.
// Called when raw data is sent over the stream, used here to detect the end of startCloseWriter().
void ClientConnection::handleWriteStream(ts::ReactiveStream& stream, int error_code, const ts::ObjectPtr& user_data)
{
    if (!ts::SysSuccess(error_code)) {
        // End of startCloseWriter (if SYS_EOF), or other error: close the connection.
        _react_client->startClose(this);
    }
}

// Implementation of ReactiveTextStreamHandlerInterface.
// Called when a text line is received.
void ClientConnection::handleTextLine(ts::ReactiveTextStream& stream, const ts::UString& line, int error_code, const ts::ObjectPtr& user_data)
{
    if (error_code == ts::SYS_EOF) {
        _opt.info(u"Session %s: end of input", _session_name);
        _state = END;
    }
    else if (ts::SysSuccess(error_code)) {
        switch (_state) {
            case START: {
                _opt.info(u"Session %s: request: %s", _session_name, line);
                _text_client.startWriteLine(nullptr, u"HTTP/1.0 204 No Content");
                _text_client.startWriteLine(nullptr, u"Server: Same-Reactor-Server");
                _text_client.startWriteLine(nullptr, u"Connection: close");
                _text_client.startWriteLine(nullptr, u"X-Session-Name: " + _session_name);
                _text_client.startWriteLine(nullptr, u"X-Echo-Request: " + line);
                _state = HEADERS;
                break;
            }
            case HEADERS: {
                if (line.empty()) {
                    // End of headers, terminate the response.
                    _state = CONTENT;
                    _text_client.startWriteLine(nullptr, u"");
                    _react_client->startCloseWriter(this);
                }
                else {
                    _opt.info(u"Session %s: header: %s", _session_name, line);
                    _text_client.startWriteLine(nullptr, u"X-Echo-Header: " + line);
                    if (line.contains(u"X-Status:")) {
                        // Return the current status of the server.
                        _text_client.startWriteLine(nullptr, ts::UString::Format(u"X-Current-Clients: %d", _server.connectedClientCount()));
                        _text_client.startWriteLine(nullptr, ts::UString::Format(u"X-Total-Clients: %d", _server.totalClientCount()));
                    }
                    else if (line.contains(u"X-Exit:")) {
                        // Exit server when the last client disconnects.
                        _server.exit();
                        _text_client.startWriteLine(nullptr, u"X-Message: exiting server");
                    }
                }
                break;
            }
            case CONTENT: {
                _opt.info(u"Session %s: content: %s", _session_name, line);
                break;
            }
            case END: {
                _opt.error(u"Session %s: received line after EOF...", _session_name);
                break;
            }
            default: {
                _opt.error(u"Session %s: shouldn't get there", _session_name);
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Server control class.
// This class serves two purposes:
// - Create application-specific object for each new incoming client session.
// - Be notified of server termination.
//----------------------------------------------------------------------------

class ServerControl:
    public ts::ReactiveServerFactoryInterface,
    public ts::ReactiveServerHandlerInterface
{
    TS_NOBUILD_NOCOPY(ServerControl);
public:
    // Constructor.
    ServerControl(ts::Reactor& reactor, CommandOptions& opt);

    // Implementation of ReactiveServerFactoryInterface.
    virtual ts::ReactiveServerSessionInterface* newClientSession(ts::ReactiveServer& server) override;

    // Implementation of ReactiveServerHandlerInterface.
    virtual void handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data) override;

private:
    ts::Reactor& _reactor;
    CommandOptions& _opt;
};

// Constructor.
ServerControl::ServerControl(ts::Reactor& reactor, CommandOptions& opt) :
    _reactor(reactor),
    _opt(opt)
{
    _opt.info(u"Server listens on %s", _opt.server_address);
}

// Implementation of ReactiveServerFactoryInterface.
// Called by ReactiveServer when it needs a new client session object.
ts::ReactiveServerSessionInterface* ServerControl::newClientSession(ts::ReactiveServer& server)
{
    return new ClientConnection(_reactor, server, _opt);
}

// Implementation of ReactiveServerHandlerInterface.
// Called after the ReactiveServer exits.
void ServerControl::handleServerExited(ts::ReactiveServer& server, const ts::ObjectPtr& user_data)
{
    _opt.info(u"Server exited");
}


//----------------------------------------------------------------------------
// Application entry point.
//----------------------------------------------------------------------------

int MainCode(int argc, char* argv[])
{
    // Decode command line options.
    CommandOptions opt(argc, argv);

    // Define all server elements. Use either clear or TLS reactive server, based on command line options.
    ts::Reactor reactor(&opt);
    ts::TCPServer tcp_server(&opt);
    std::unique_ptr<ts::ReactiveTCPServer> react_server = opt.tls_args.use_tls ?
        std::make_unique<ts::ReactiveTLSServer>(reactor, tcp_server, opt.tls_args) :
        std::make_unique<ts::ReactiveTCPServer>(reactor, tcp_server);
    ts::ReactiveServer server(*react_server);
    ServerControl control(reactor, opt);

    // Initialize all server elements.
    if (!reactor.open() ||
        !tcp_server.open(ts::IP::v6) ||
        !tcp_server.reusePort(opt.tls_args.reuse_port) ||
        !tcp_server.bind(opt.server_address) ||
        !tcp_server.listen(5) ||
        !server.start(&control, &control))
    {
        return EXIT_FAILURE;
    }

    // Automatically exit reactor's event loop when the server terminates.
    server.setExitEventLoop(true);

    // Process events.
    reactor.processEventLoop();

    // Event loop is exited when the server exits.
    reactor.close();
    return EXIT_SUCCESS;
}
