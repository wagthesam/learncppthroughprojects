#include "websocket-client.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <iostream>
#include <utility>

namespace NetworkMonitor {
    WebSocketClient::WebSocketClient(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc) : 
        url_(url),
        endpoint_(endpoint),
        port_(port),
        resolver_(boost::asio::make_strand(ioc)),
        ws_(boost::asio::make_strand(ioc)) {}

    WebSocketClient::~WebSocketClient() {
        if (ws_.is_open()) {
                Log("WebSocketClient being destroyed without closing ws. Closing ws...");
            Close([this](auto ec){
                Log("ws closed");
            });
        }
    }

    void WebSocketClient::Connect(
        std::function<void (boost::system::error_code)> onConnect,
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage,
        std::function<void (boost::system::error_code)> onDisconnect) {
        onConnect_ = onConnect;
        onMessage_ = onMessage;
        onDisconnect_ = onDisconnect;
        boost::system::error_code ec {};
        resolver_.async_resolve(
            url_,
            port_,
            [this](auto&& ec, auto&& results) {
                OnResolve(std::forward<decltype(ec)>(ec), std::forward<decltype(results)>(results));
            });
    }

    void WebSocketClient::OnResolve(
        const boost::system::error_code& ec,
        boost::asio::ip::tcp::resolver::results_type results) {
        if (ec) {
            Log("OnResolve", ec);
            if (onConnect_) {
                onConnect_(ec);
            }
            return;
        }

        ws_.next_layer().async_connect(
            *results,
            [this](auto&& ec) {
                OnConnect(std::forward<decltype(ec)>(ec));
            });
    }

    void WebSocketClient::OnConnect(
        const boost::system::error_code& ec) {
        if (ec) {
            Log("OnConnect", ec);
            if (onConnect_) {
                onConnect_(ec);
                onDisconnect_(ec);
            }
            return;
        }
        ws_.async_handshake(
            url_,
            endpoint_,
            [this](auto&& ec) {
                OnHandshake(std::forward<decltype(ec)>(ec));
            }
        );
    }

    void WebSocketClient::OnHandshake(
        const boost::system::error_code& ec) {
        if (ec) {
            Log("OnHandshake", ec);
            onConnect_(ec);
            onDisconnect_(ec);
            return;
        }
        ws_.text(true);
        onConnect_(ec);

        ws_.async_read(
            buffer_,
            [this](auto&& ec, auto&& bytes_transferred) {
                OnRead(
                    std::forward<decltype(ec)>(ec),
                    std::forward<decltype(bytes_transferred)>(bytes_transferred));
            }
        );
    }

    void WebSocketClient::OnRead(
        const boost::system::error_code& ec,
        std::size_t bytes_transferred) {
        if (ec) {
            Log("OnRead", ec);
            return;
        }
        onMessage_(ec, boost::beast::buffers_to_string(buffer_.data()));
        buffer_.clear();
        ws_.async_read(
            buffer_,
            [this](auto&& ec, auto&& bytes_transferred) {
                    OnRead(
                        std::forward<decltype(ec)>(ec),
                        std::forward<decltype(bytes_transferred)>(bytes_transferred));
                }
        );
    }

    void WebSocketClient::Send(
        const std::string& message,
        std::function<void (boost::system::error_code)> onSend) {
        ws_.async_write(boost::asio::buffer(message), [this, onSend](auto ec, auto bytes_transferred){
            if (ec) {
                Log("Send", ec);
                return;
            }
            onSend(ec);
        });
    }

    void WebSocketClient::Close(
        std::function<void (boost::system::error_code)> onClose) {
        ws_.async_close(boost::beast::websocket::close_code::none, [this, onClose](auto ec){
            if (ec) {
                Log("Close", ec);
                return;
            }
            onClose(ec);
        });
    }

    void WebSocketClient::Log(const std::string& ref, const boost::system::error_code& ec) {
        std::cout << ref << " > "
                << (ec ? "Error: " : "OK!")
                << (ec ? ec.message() : "")
                << std::endl;
    }

    void WebSocketClient::Log(const std::string& msg) {
        std::cout << msg << std::endl;
    }
}