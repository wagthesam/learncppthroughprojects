#pragma once

#include "network-monitor/websocket-client.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <string>
#include <iostream>

namespace NetworkMonitor {

class MockTcpStream : public boost::beast::tcp_stream {
public:
    using boost::beast::tcp_stream::tcp_stream;
    inline static boost::system::error_code connectEc = {};

    template <class ConnectHandler>
    void async_connect(
        endpoint_type const& ep,
        ConnectHandler&& handler
    ) {
        boost::asio::async_initiate<
                ConnectHandler,
                void(boost::system::error_code)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler = std::move(handler)]() {
                        handler(MockTcpStream::connectEc);
                    }
                );
            },
            handler,
            get_executor()
        );
    }
};

template <typename NextLayer>
class MockSslStream : public boost::beast::ssl_stream<NextLayer> {
public:
    using boost::beast::ssl_stream<NextLayer>::ssl_stream;
    inline static boost::system::error_code handshakeEc = {};

    template<typename HandshakeHandler>
    void async_handshake(
        boost::asio::ssl::stream_base::handshake_type type,
        HandshakeHandler handler
    ) {
        boost::asio::async_initiate<
                HandshakeHandler,
                void(boost::system::error_code)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler = std::move(handler)]() {
                        handler(MockSslStream::handshakeEc);
                    }
                );
            },
            handler,
            this->get_executor()
        );
    }
};

template <typename NextLayer>
class MockWebsocketStream : public boost::beast::websocket::stream<NextLayer> {
public:
    using boost::beast::websocket::stream<NextLayer>::stream;
    inline static boost::system::error_code handshakeEc = {};
    inline static boost::system::error_code writeEc = {};

    template<typename HandshakeHandler>
    void async_handshake(
        std::string_view host,
        std::string_view target,
        HandshakeHandler&& handler) 
    {
        boost::asio::async_initiate<
                HandshakeHandler,
                void(boost::system::error_code)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler = std::move(handler)]() {
                        handler(MockWebsocketStream::handshakeEc);
                    }
                );
            },
            handler,
            this->get_executor()
        );
    }

    template <typename Buffer, typename WriteHandler>
    void async_write(
        const Buffer & buffer,
        WriteHandler&& handler
        ) {
        boost::asio::async_initiate<
                WriteHandler,
                void(
                    boost::system::error_code,
                    std::size_t)>(
            [buffer = std::move(buffer)](auto&& handler, auto&& ex) {
                if (MockWebsocketStream::writeEc) {
                    boost::asio::post(
                        ex, 
                        [handler = std::move(handler)]() {
                            handler(MockWebsocketStream::writeEc, 0);
                        }
                    );
                } else {
                    auto bufferSize = buffer.size();
                    boost::asio::post(
                        ex, 
                        [bufferSize, handler = std::move(handler)]() {
                            handler(MockWebsocketStream::writeEc, bufferSize);
                        }
                    );
                }
            },
            handler,
            this->get_executor()
        );
    }
};

template <typename TeardownHandler>
void async_teardown(
    boost::beast::role_type role,
    MockSslStream<MockTcpStream>& socket,
    TeardownHandler&& handler
) {
    return;
}

template <typename TeardownHandler>
void async_teardown(
    boost::beast::role_type role,
    MockTcpStream& socket,
    TeardownHandler&& handler
) {
    return;
}

class MockResolver {
public:
    inline static boost::system::error_code resolveEc = {};

    template <typename ExecutionContext>
    MockResolver(const ExecutionContext& context) : context_(context) {}
    
    template <typename ResolveHandler>
    void async_resolve(
        const std::string& host,
        const std::string& service,
        ResolveHandler&& handler)
    {
        boost::asio::async_initiate<
                ResolveHandler,
                void(
                    boost::system::error_code,
                    boost::asio::ip::tcp::resolver::results_type)>(
            [&host, &service](auto&& handler, auto&& context) {
                if (MockResolver::resolveEc) {
                    auto results = boost::asio::ip::tcp::resolver::results_type();
                    boost::asio::post(
                        context, 
                        [results = std::move(results), handler = std::move(handler)]() {
                            handler(MockResolver::resolveEc, results);
                        }
                    );
                } else {
                    boost::asio::ip::tcp::endpoint endpoint(
                        boost::asio::ip::make_address("127.0.0.1"),
                        443);
                    boost::asio::ip::tcp::resolver::results_type results = 
                        boost::asio::ip::tcp::resolver::results_type::create(
                            endpoint,
                            host,
                            service
                        );
                    boost::asio::post(
                        context, 
                        [results = std::move(results), handler = std::move(handler)]() {
                            handler(MockResolver::resolveEc, results);
                        }
                    );
                }
            },
            handler,
            context_
        );
    }
private:
    boost::asio::strand<boost::asio::io_context::executor_type> context_;
};


/*! \brief Type alias for the mocked WebSocketClient.
 *
 *  For now we only mock the DNS resolver.
 */
using MockTlsStream = MockSslStream<MockTcpStream>;
using MockWsStream = MockWebsocketStream<MockTlsStream>;
using TestWebSocketClient = NetworkMonitor::WebSocketClient<MockResolver, MockWsStream>;

}
