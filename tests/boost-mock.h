#pragma once

#include "network-monitor/websocket-client.h"

#include <boost/asio.hpp>

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
using TestWebSocketClient = NetworkMonitor::WebSocketClient<
    MockResolver,
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<MockTcpStream>
    >
>;
}


/*

    template <typename ConstBufferSequence>
    void async_write(
        const ConstBufferSequence & buffers,
        ) {
        boost::asio::async_initiate<
                ResolveHandler,
                void(
                    boost::system::error_code,
                    std::size_t)>(
            [](auto&& handler, auto&& context) {
                if (MockResolver::resolveEc) {
                    std::size_t result_size = 0;
                    boost::asio::post(
                        context, 
                        [result_size, handler = std::move(handler)]() {
                            handler(MockResolver::resolveEc, result_size);
                        }
                    );
                } else {
                    // pass for now
                }
            },
            handler,
            context_
        );
    }
*/