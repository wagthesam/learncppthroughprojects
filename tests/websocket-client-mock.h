#pragma once

#include "network-monitor/websocket-client.h"
#include "network-monitor/stomp-frame.h"
#include "network-monitor/stomp-client.h"

namespace NetworkMonitor {

class WebSocketClientMock {
public:
    WebSocketClientMock(
        std::string_view url,
        std::string_view endpoint,
        std::string_view port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx
    ) : context_(boost::asio::make_strand(ioc)) {}

    ~WebSocketClientMock() {
        Close();
    }

    void Connect(
        std::function<void (boost::system::error_code)> onConnect = nullptr,
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage = nullptr,
        std::function<void (boost::system::error_code)> onDisconnect = nullptr
    ) {
        onMessage_ = onMessage;
        onDisconnect_ = onDisconnect;
        closed_ = false;
        boost::asio::async_initiate<
                std::function<void (boost::system::error_code)>,
                void(boost::system::error_code)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler = std::move(handler)]() {
                        handler(boost::system::error_code{});
                    }
                );
            },
            onConnect,
            context_
        );
    }

    void Send(
        const std::string& message,
        std::function<void (boost::system::error_code)> onSend = nullptr
    ) {
        if (!closed_) {
            boost::asio::async_initiate<
                    std::function<void (boost::system::error_code)>,
                    void(boost::system::error_code)>(
                [this](auto&& handler, auto&& message, auto&& ex) {
                    boost::asio::post(
                        ex,
                        [this, handler = std::move(handler), message = std::move(message)]() {
                            SendResponses();
                            handler(boost::system::error_code{});
                        }
                    );
                },
                onSend,
                message,
                context_
            );
        } else {
            boost::asio::async_initiate<
                    std::function<void (boost::system::error_code)>,
                    void(boost::system::error_code)>(
                [this](auto&& handler, auto&& message, auto&& ex) {
                    boost::asio::post(
                        ex,
                        [this, handler = std::move(handler), message = std::move(message)]() {
                            handler(boost::asio::error::operation_aborted);
                        }
                    );
                },
                onSend,
                message,
                context_
            );
        }
    }

    void Close(
        std::function<void (boost::system::error_code)> onClose = nullptr
    ) {
        boost::asio::async_initiate<
                std::function<void (boost::system::error_code)>,
                void(boost::system::error_code)>(
            [](auto&& handler, auto&& ex) {
                boost::asio::post(
                    ex,
                    [handler = std::move(handler)]() {
                        if (handler) {
                            handler(boost::system::error_code{});
                        }
                    }
                );
            },
            onClose,
            context_
        );
        closed_ = true;
    }

    virtual void SendResponses() {}
protected:
    boost::asio::strand<boost::asio::io_context::executor_type> context_;
    bool closed_ = true;
    std::function<void (boost::system::error_code, std::string&&)> onMessage_;
    std::function<void (boost::system::error_code)> onDisconnect_;
};


class MockWebSocketClientForStomp : public WebSocketClientMock {
public:

    using WebSocketClientMock::WebSocketClientMock;

    void SendResponses() {
        for (auto& msg : messages_) {
            if (!closed_) {
                if (msg.substr(0,5) == "ERROR") {
                    Close();
                    onDisconnect_(boost::system::error_code{});
                }
                boost::asio::async_initiate<
                        std::function<void (boost::system::error_code, std::string&&)>,
                        void(boost::system::error_code, std::string&&)>(
                    [](auto&& handler, auto&& message, auto&& ex) {
                        boost::asio::post(
                            ex,
                            [handler, message = std::move(message)]() mutable {
                                handler(boost::system::error_code{}, std::move(message));
                            }
                        );
                    },
                    onMessage_,
                    std::move(msg),
                    context_
                );
            }
        }
        messages_.clear();
    }

    inline static std::vector<std::string> messages_ {};
};

using MockStompClient = NetworkMonitor::StompClient<MockWebSocketClientForStomp>;
}