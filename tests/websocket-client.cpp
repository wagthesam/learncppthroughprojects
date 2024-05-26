#include "boost-mock.h"
#include "network-monitor/websocket-client.h"

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <sstream>

#include "absl/strings/match.h"

namespace {
    auto CheckResponse(const std::string& response) -> bool
    {
        // We do not parse the whole message. We only check that it contains some
        // expected items.
        bool ok {true};
        ok &= absl::StrContains(response, "ERROR");
        ok &= absl::StrContains(response, "ValidationInvalidAuth");
        return ok;
    }
}

using NetworkMonitor::BoostWebSocketClient;
using NetworkMonitor::MockResolver;
using NetworkMonitor::TestWebSocketClient;

struct WebSocketClientTestFixture {
    WebSocketClientTestFixture()
    {
        NetworkMonitor::MockResolver::resolveEc = {};
        NetworkMonitor::MockTcpStream::connectEc = {};
        NetworkMonitor::MockTlsStream::handshakeEc = {};
        NetworkMonitor::MockWsStream::handshakeEc = {};
    }
};

using timeout = boost::unit_test::timeout;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_WebSocketClient_non_mock);

BOOST_AUTO_TEST_CASE(class_WebSocketClient)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
    // Connection targets
    const std::string url {"ltnm.learncppthroughprojects.com"};
    const std::string endpoint {"/echo"};
    const std::string port {"443"};
    const std::string message {"Hello WebSocket"};

    // Always start with an I/O context object.
    boost::asio::io_context ioc {};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);

    // The class under test
    BoostWebSocketClient client {url, endpoint, port, ioc, ctx};

    // We use these flags to check that the connection, send, receive functions
    // work as expected.
    bool connected {false};
    bool messageSent {false};
    bool messageReceived {false};
    bool messageMatches {false};
    bool disconnected {false};

    // Our own callbacks
    auto onSend {[&messageSent](auto ec) {
        messageSent = !ec;
    }};
    auto onConnect {[&client, &connected, &onSend, &message](auto ec) {
        connected = !ec;
        if (!ec) {
            client.Send(message, onSend);
        }
    }};
    auto onClose {[&disconnected](auto ec) {
        disconnected = !ec;
    }};
    auto onReceive {[&client,
                      &onClose,
                      &messageReceived,
                      &messageMatches,
                      &message](auto ec, auto received) {
        messageReceived = !ec;
        messageMatches = message == received;
        client.Close(onClose);
    }};

    // We must call io_context::run for asynchronous callbacks to run.
    client.Connect(onConnect, onReceive);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    bool ok {
        connected &&
        messageSent &&
        messageReceived &&
        messageMatches &&
        disconnected
    };

    BOOST_TEST(connected);
    BOOST_TEST(messageSent);
    BOOST_TEST(messageReceived);
    BOOST_TEST(messageMatches);
    BOOST_TEST(disconnected);
    BOOST_TEST(ok);
}

BOOST_AUTO_TEST_CASE(class_Stomp_Msg)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
    // Connection targets
    const std::string url {"ltnm.learncppthroughprojects.com"};
    const std::string endpoint {"/network-events"};
    const std::string port {"443"};

    const std::string username {"fake_username"};
    const std::string password {"fake_password"};
    std::stringstream ss {};
    ss << "STOMP" << '\n'
       << "accept-version:1.2" << '\n'
       << "host:transportforlondon.com" << '\n'
       << "login:" << username << '\n'
       << "passcode:" << password << '\n'
       << '\n' // Headers need to be followed by a blank line.
       << '\0'; // The body (even if absent) must be followed by a NULL octet.
    const std::string message {ss.str()};
    std::string response;

    // Always start with an I/O context object.
    boost::asio::io_context ioc {};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);

    // The class under test
    BoostWebSocketClient client {url, endpoint, port, ioc, ctx};

    // We use these flags to check that the connection, send, receive functions
    // work as expected.
    bool connected {false};
    bool messageSent {false};
    bool messageReceived {false};
    bool disconnected {false};

    // Our own callbacks
    auto onSend {[&messageSent](auto ec) {
        messageSent = !ec;
    }};
    auto onConnect {[&client, &connected, &onSend, &message](auto ec) {
        connected = !ec;
        if (!ec) {
            client.Send(message, onSend);
        }
    }};
    auto onClose {[&disconnected](auto ec) {
        disconnected = !ec;
    }};
    auto onReceive {[&client,
                      &onClose,
                      &messageReceived,
                      &response](auto ec, auto received) {
        messageReceived = !ec;
        response = received;
        client.Close(onClose);
    }};

    // We must call io_context::run for asynchronous callbacks to run.
    client.Connect(onConnect, onReceive);
    ioc.run();

    BOOST_CHECK(connected);
    BOOST_CHECK(messageSent);
    BOOST_CHECK(messageReceived);
    BOOST_CHECK(disconnected);
    BOOST_CHECK(CheckResponse(response));
}
BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(class_WebSocketClient_mock);

BOOST_FIXTURE_TEST_SUITE(Connect, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(fail_resolve, *timeout {1})
{
    // We use the mock client so we don't really connect to the target.
    const std::string url {"some.echo-server.com"};
    const std::string endpoint {"/"};
    const std::string port {"443"};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context ioc {};

    // Set the expected error codes.
    MockResolver::resolveEc = boost::asio::error::host_not_found;

    TestWebSocketClient client {url, endpoint, port, ioc, ctx};
    bool calledOnConnect {false};
    auto onConnect {[&calledOnConnect](auto ec) {
        calledOnConnect = true;
        BOOST_CHECK_EQUAL(ec, boost::asio::error::host_not_found);
    }};
    client.Connect(onConnect);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(calledOnConnect);
}

BOOST_AUTO_TEST_CASE(fail_socket_connect, *timeout {1})
{
    // We use the mock client so we don't really connect to the target.
    const std::string url {"some.echo-server.com"};
    const std::string endpoint {"/"};
    const std::string port {"443"};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context ioc {};

    // Set the expected error codes.
    NetworkMonitor::MockTcpStream::connectEc = boost::asio::error::connection_refused;

    TestWebSocketClient client {url, endpoint, port, ioc, ctx};
    bool calledOnConnect {false};
    auto onConnect {[&calledOnConnect](auto ec) {
        calledOnConnect = true;
        BOOST_CHECK_EQUAL(ec, boost::asio::error::connection_refused);
    }};
    client.Connect(onConnect);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(calledOnConnect);
}

BOOST_AUTO_TEST_CASE(fail_ssl_handshake, *timeout {1})
{
    // We use the mock client so we don't really connect to the target.
    const std::string url {"some.echo-server.com"};
    const std::string endpoint {"/"};
    const std::string port {"443"};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context ioc {};

    // Set the expected error codes.
    NetworkMonitor::MockTlsStream::handshakeEc = boost::asio::error::timed_out;

    TestWebSocketClient client {url, endpoint, port, ioc, ctx};
    bool calledOnConnect {false};
    auto onConnect {[&calledOnConnect](auto ec) {
        calledOnConnect = true;
        BOOST_CHECK_EQUAL(ec, boost::asio::error::timed_out);
    }};
    client.Connect(onConnect);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(calledOnConnect);
}

BOOST_AUTO_TEST_CASE(fail_websocket_handshake, *timeout {1})
{
    // We use the mock client so we don't really connect to the target.
    const std::string url {"some.echo-server.com"};
    const std::string endpoint {"/"};
    const std::string port {"443"};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context ioc {};

    // Set the expected error codes.
    NetworkMonitor::MockWsStream::handshakeEc = boost::asio::error::timed_out;

    TestWebSocketClient client {url, endpoint, port, ioc, ctx};
    bool calledOnConnect {false};
    auto onConnect {[&calledOnConnect](auto ec) {
        calledOnConnect = true;
        BOOST_CHECK_EQUAL(ec, boost::asio::error::timed_out);
    }};
    client.Connect(onConnect);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(calledOnConnect);
}

BOOST_AUTO_TEST_CASE(success_websocket_connection, *timeout {1})
{
    // We use the mock client so we don't really connect to the target.
    const std::string url {"some.echo-server.com"};
    const std::string endpoint {"/"};
    const std::string port {"443"};

    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context ioc {};

    TestWebSocketClient client {url, endpoint, port, ioc, ctx};
    bool calledOnConnect {false};
    auto onConnect {[&calledOnConnect](auto ec) {
        calledOnConnect = true;
        BOOST_CHECK_EQUAL(ec, boost::system::error_code());
    }};
    client.Connect(onConnect);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(calledOnConnect);
}

BOOST_AUTO_TEST_SUITE_END(); // Connect

BOOST_AUTO_TEST_SUITE_END(); // class_WebSocketClient

BOOST_AUTO_TEST_SUITE_END(); // network_monitor
