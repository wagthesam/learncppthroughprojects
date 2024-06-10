#include "network-monitor/network-monitor.h"
#include "websocket-client-mock.h"

static const std::string USERNAME = std::getenv("stomp_username");
static const std::string PASSWORD = std::getenv("stomp_password");

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(class_NetworkMonitor)
{


    const std::string url {"some.echo-server.com"};
    const std::string endpoint {"/passengers"};
    const std::string port {"443"};
    boost::asio::ssl::context ctx {boost::asio::ssl::context::tlsv12_client};
    ctx.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context ioc {};
    NetworkMonitor::MockStompClient client {
        url,
        endpoint,
        port,
        ioc,
        ctx
    };

    const std::string username {"user"};
    const std::string password {"password"};
    bool connected = false;
    bool disconnected = false;

    std::string connectedFrame {
        "CONNECTED\n"
        "version:1.2\n"
        "session:12\n"
        "\n"
        "\0"s
    };
    const std::vector<std::string> connectedMock {
        connectedFrame
    };
    NetworkMonitor::MockWebSocketClientForStomp::messages_ = connectedMock;

    client.Connect(
        username,
        password,
        [&connected](NetworkMonitor::StompClientError error, std::string&& msg) {
            if (error == NetworkMonitor::StompClientError::kOk) {
                connected = true;
            }
        },
        [&disconnected](NetworkMonitor::StompClientError error, std::string&& msg) {
            if (error == NetworkMonitor::StompClientError::kOk) {
                disconnected = true;
            }
        });
    ioc.run();
    ioc.reset();
    BOOST_CHECK(connected);
    BOOST_CHECK(!disconnected);
    BOOST_CHECK(client.IsConnected());
    BOOST_CHECK(!client.IsSubscribed());

    std::vector<std::string> messages;
    bool subscribed = false;
    const auto subscribeToken = client.Subscribe(
        [&subscribed](NetworkMonitor::StompClientError error, std::string&& msg) {
            if (error == NetworkMonitor::StompClientError::kOk) {
                subscribed = true;
            }
        },
        [&messages](NetworkMonitor::StompClientError error, std::string&& msg) {
            if (error == NetworkMonitor::StompClientError::kOk) {
                messages.emplace_back(msg);
            }
        }
    );
    std::stringstream ssReceipt;
    ssReceipt << "RECEIPT\n"
        << "receipt-id:" << subscribeToken.receiptId << "\n"
        << "\n" << "\0"s;
    const std::vector<std::string> messagesMock {
        ssReceipt.str()
    };
    NetworkMonitor::MockWebSocketClientForStomp::messages_ = messagesMock;
    ioc.run();
    ioc.reset();
    BOOST_CHECK(subscribed);
    BOOST_CHECK(client.IsConnected());
    BOOST_CHECK(client.IsSubscribed());

    std::stringstream ssMessage1;
    ssMessage1 << "MESSAGE\n"
        << "subscription:" << subscribeToken.subscriptionId << "\n"
        << "message-id:001\n"
        << "destination:/passengers\n"
        << "content-length:11\n"
        << "content-type:text/plain\n"
        << "\n"
        << "hello queue"
        << "\0"s;
    std::stringstream ssMessage2;
    ssMessage2 << "MESSAGE\n"
        << "subscription:" << subscribeToken.subscriptionId << "\n"
        << "message-id:002\n"
        << "destination:/passengers\n"
        << "content-length:11\n"
        << "content-type:text/plain\n"
        << "\n"
        << "hello queu3"
        << "\0"s;
    std::stringstream ssMessage3;
    ssMessage3 << "MESSAGE\n"
        << "subscription:" << subscribeToken.subscriptionId << "\n"
        << "message-id:003\n"
        << "destination:/passengers\n"
        << "content-length:11\n"
        << "content-type:text/plain\n"
        << "\n"
        << "hello queu4"
        << "\0"s;
        
    const std::vector<std::string> inMessagesMock {
        ssMessage1.str(),
        ssMessage2.str(),
        ssMessage3.str()
    };
        
    const std::vector<std::string> messagesCheck {
        "hello queue",
        "hello queu3",
        "hello queu4"
    };
    NetworkMonitor::MockWebSocketClientForStomp::messages_ = inMessagesMock;
    client.GetWsClient()->SendResponses();
    ioc.run();
    ioc.reset();

    BOOST_CHECK_EQUAL_COLLECTIONS(
        messagesCheck.begin(), messagesCheck.end(), messages.begin(), messages.end());
    

    bool closed = false;
    client.Close(
        [&closed](NetworkMonitor::StompClientError error) {
            if (error == NetworkMonitor::StompClientError::kOk) {
                closed = true;
            }
        }
    );
    ioc.run();
    ioc.reset();

    BOOST_CHECK(!client.IsConnected());
    BOOST_CHECK(!client.IsSubscribed());
    BOOST_CHECK(!disconnected);
}

BOOST_AUTO_TEST_SUITE_END(); // network_monitor