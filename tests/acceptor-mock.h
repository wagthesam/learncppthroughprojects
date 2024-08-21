#pragma once

#include "network-monitor/websocket-server.h"

#include <queue>

namespace NetworkMonitor {

class AcceptorMock {
public:
    inline static boost::system::error_code openEc = {};
    inline static boost::system::error_code bindEc = {};
    inline static boost::system::error_code listenEc = {};
    inline static std::queue<boost::system::error_code> ec_queue = {};

    template <typename ExecutionContext>
    AcceptorMock(
        ExecutionContext&& context
    ) : context_{context} {}

    ~AcceptorMock() {}

    template <typename ProtocolType>
    void Open(
        const ProtocolType& protocol,
        boost::system::error_code& ec) {
        ec = openEc;
    }
    
    template<typename SettableSocketOption>
    void set_option(
        const SettableSocketOption & option,
        boost::system::error_code & ec) {
        ec = {};
    }

    template <typename EndpointType>
    void bind(
        const EndpointType& endpoint,
        boost::system::error_code & ec) {
        ec = bindEc;
    }

    void listen(
        int backlog,
        boost::system::error_code & ec
        ) {
        ec = listenEc;
    }

    template <typename ExecutorContext, typename AcceptHandler>
    void async_accept(
        const ExecutorContext& ioc,
        AcceptHandler&& onAccept
    ) {
        boost::asio::async_initiate<
            AcceptHandler,
            void(boost::system::error_code, boost::asio::ip::tcp::socket&&)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler, ex]() {
                        if (!ec_queue.empty()) {
                            auto ec {ec_queue.front()};
                            ec_queue.pop();
                            handler(ec, boost::asio::ip::tcp::socket {ex});
                        }
                    }
                );
            },
            onAccept,
            ioc
        );
    }
    
protected:
    boost::asio::strand<boost::asio::io_context::executor_type> context_;
};

}