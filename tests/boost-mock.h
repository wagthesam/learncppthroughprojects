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
    inline static boost::system::error_code readEc = {};
    inline static boost::system::error_code closeEc = {};
    inline static boost::system::error_code acceptEc = {};
    inline static std::string readBuffer = {};

    template<typename HandshakeHandler>
    void async_handshake(
        std::string_view host,
        std::string_view target,
        HandshakeHandler&& handler) 
    {
        boost::asio::async_initiate<
                HandshakeHandler,
                void(boost::system::error_code)>(
            [](auto&& handler, auto stream) {
                boost::asio::post(
                    stream->get_executor(),
                    [handler = std::move(handler), stream]() {
                        stream->closed_ = false;
                        handler(MockWebsocketStream::handshakeEc);
                    }
                );
            },
            handler,
            this
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

    template <typename DynamicBuffer, typename ReadHandler>
    void async_read(
        DynamicBuffer& buffer,
        ReadHandler&& handler
        ) {
        boost::asio::async_initiate<
                ReadHandler,
                void(
                    boost::system::error_code,
                    std::size_t)>(
            [this](auto&& handler, auto& buffer) {
                RecursiveRead(buffer, std::forward<ReadHandler>(handler));
            },
            handler,
            buffer
        );
    }

    template <typename CloseHandler>
    void async_close(
            boost::beast::websocket::close_reason const& cr,
            CloseHandler&& handler
        ) {
        boost::asio::async_initiate<
                CloseHandler,
                void(boost::system::error_code)>(
            [](auto&& handler, auto stream) {
                boost::asio::post(
                    stream->get_executor(),
                    [handler = std::move(handler), stream]() {
                        stream->closed_ = true;
                        handler(MockWebsocketStream::closeEc);
                    }
                );
            },
            handler,
            this
        );
    }

    template<typename AcceptHandler>
    void async_accept(
        AcceptHandler handler
    ) {
        boost::asio::async_initiate<
                AcceptHandler,
                void(boost::system::error_code)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler = std::move(handler)]() {
                        handler(MockWebsocketStream::acceptEc);
                    }
                );
            },
            handler,
            this->get_executor()
        );
    }

private:
    template <typename DynamicBuffer, typename ReadHandler>
    void RecursiveRead(
        DynamicBuffer& buffer,
        ReadHandler&& handler
    ) {
        if (closed_) {
            boost::asio::post(
                this->get_executor(),
                [handler = std::move(handler)]() {
                    handler(boost::asio::error::operation_aborted, 0);
                }
            );
        } else {
            size_t readSize = MockWebsocketStream::readBuffer.size();
            readSize = boost::asio::buffer_copy(
                buffer.prepare(MockWebsocketStream::readBuffer.size()),
                boost::asio::buffer(MockWebsocketStream::readBuffer)
            );
            buffer.commit(readSize);
            MockWebsocketStream::readBuffer = "";

            if (readSize > 0) {
                boost::asio::post(
                    this->get_executor(),
                    [readSize, handler = std::move(handler)]() {
                        handler(MockWebsocketStream::readEc, readSize);
                    }
                );
            } else {
                boost::asio::post(
                    this->get_executor(),
                    [this, &buffer, handler = std::move(handler)]() {
                        RecursiveRead(buffer, handler);
                    }
                );
            }
        }
    }

    bool closed_ {true};
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
