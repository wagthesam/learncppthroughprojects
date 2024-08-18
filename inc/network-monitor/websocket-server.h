#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <ctime>


namespace NetworkMonitor {

static std::string generateRandomString(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string randomString;
    
    std::srand(std::time(0)); // Use current time as seed for random generator
    
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[std::rand() % characters.length()];
    }
    
    return randomString;
}

template <typename WebSocketStream>
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession<WebSocketStream>> {
public:
    WebSocketSession(
            boost::asio::ip::tcp::socket socket,
            boost::asio::ssl::context& ctx,
            std::function<void (boost::system::error_code, const std::string&)> onConnect = nullptr,
            std::function<std::vector<std::string> (boost::system::error_code,
                                                    std::string&&,
                                                    const std::string&)> onMessage = nullptr,
            std::function<void (boost::system::error_code, const std::string&)> onDisconnect = nullptr,
            std::function<void (boost::system::error_code, const std::string&)> onSend = nullptr) : 
            ws_{std::move(socket), ctx},
            onConnect_(onConnect),
            onMessage_(onMessage),
            onDisconnect_(onDisconnect),
            onSend_(onSend),
            session_id_(generateRandomString(10)) {}

    void Init() {
        boost::beast::get_lowest_layer(ws_).expires_never();
        ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::server
        ));
        ws_.next_layer().async_handshake(
            boost::asio::ssl::stream_base::server,
            [this, self = this->shared_from_this()](auto&& ec) {
                OnTlsHandshake(ec, self);
            });
    }

    void Close(
        std::function<void (boost::system::error_code)> onClose = nullptr
    ) {
        closed_ = true;
        ws_.async_close(boost::beast::websocket::close_code::none, [this, onClose](auto ec){
            if (onClose != nullptr) {
                onClose(ec);
            }
        });
    }
private:
    void OnTlsHandshake(
        const boost::system::error_code& ec,
        std::shared_ptr<WebSocketSession> sp
    ) {
        if (ec) {
            Log("OnTlsHandshake", ec);
            if (onConnect_) {
                onConnect_(ec, session_id_);
            }
            return;
        }
        ws_.async_accept(
            [this, sp](auto&& ec){
                OnWsHandshake(ec, sp);
            });
    }

    void OnWsHandshake(
        const boost::system::error_code& ec,
        std::shared_ptr<WebSocketSession> sp
    ) {
        if (ec) {
            Log("OnWsHandshake", ec);
            if (onConnect_) {
                onConnect_(ec, session_id_);
            }
            return;
        }
        ws_.text(true);
        if (onConnect_) {
            onConnect_(ec, session_id_);
        }
        ListenToIncomingMessage(ec);
    }

    void ListenToIncomingMessage(
        const boost::system::error_code& ec
    ) {
        if (ec == boost::asio::error::operation_aborted) {
            if (onDisconnect_ && !closed_) {
                onDisconnect_(ec, session_id_);
            }
            return;
        }

        ws_.async_read(
            buffer_,
            [this](auto ec, auto bytes_transferred) {
                OnRead(ec, bytes_transferred);
                ListenToIncomingMessage(ec);
            }
        );
    }

    void OnRead(
        const boost::system::error_code& ec,
        std::size_t bytes_transferred) {
        if (ec) {
            Log("OnRead", ec);
            return;
        }
        if (onMessage_) {
            auto messages = onMessage_(ec, session_id_, boost::beast::buffers_to_string(buffer_.data()));
            for (const auto& msg : messages) {
                SendMessage(msg);
            }
        }
        buffer_.clear();
    }

    void SendMessage(
        const std::string& msg
    ) {
        ws_.async_write(boost::asio::buffer(msg), [this](auto ec, auto bytes_transferred){
            if (ec) {
                Log("Send", ec);
                return;
            }
            onSend_(ec, session_id_);
        });
    }
    void Log(std::string_view ref, const boost::system::error_code& ec) {
        std::cout << ref << " > "
                << (ec ? "Error: " : "OK!")
                << (ec ? ec.message() : "")
                << std::endl;
    }

    WebSocketStream ws_;
    std::function<void (boost::system::error_code, const std::string&)> onConnect_ {nullptr};
    std::function<std::vector<std::string> (boost::system::error_code,
                                            std::string&&,
                                            const std::string&)> onMessage_ {nullptr};
    std::function<void (boost::system::error_code, const std::string&)> onDisconnect_ {nullptr};
    std::function<void (boost::system::error_code, const std::string&)> onSend_ {nullptr};
    bool closed_{false};
    std::string session_id_;
    
    boost::beast::flat_buffer buffer_;
};

template <typename WebSocketStream>
class WebSocketServer {
public:
    WebSocketServer(
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx,
        boost::asio::ip::tcp::acceptor& acceptor,
        boost::asio::ip::tcp::endpoint& endpoint
    ) : ioc_(ioc), ctx_(ctx), acceptor_(std::move(acceptor)), endpoint_(std::move(endpoint)) {}

    void Init(
        std::function<void (boost::system::error_code, const std::string&)> onConnect = nullptr,
        std::function<std::vector<std::string> (boost::system::error_code,
                                                std::string&&,
                                                const std::string&)> onMessage = nullptr,
        std::function<void (boost::system::error_code, const std::string&)> onDisconnect = nullptr,
        std::function<void (boost::system::error_code, const std::string&)> onSend = nullptr) {
        onConnect_ = onConnect;
        onDisconnect_ = onDisconnect;
        onMessage_ = onMessage;
        onSend_ = onSend;
        acceptor_.open(endpoint_.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint_);
        acceptor_.listen();
        DoAccept();
    }
private:
    void DoAccept() {
        acceptor_.async_accept(ioc_, 
            [this](auto&& ec, auto&& socket) {
                OnAsyncAccept(ec, socket);
            }
        );
        return;
    }

    void OnAsyncAccept(
        const boost::system::error_code& ec,
        boost::asio::ip::tcp::socket socket
    ) {
        boost::asio::post(ioc_, 
            boost::asio::bind_executor(
                ioc_.get_executor(), 
                std::bind(
                    &WebSocketServer::CreateSession,
                    this,
                    ec,
                    socket)
            )
        );
        DoAccept();
    }

    void CreateSession(
        const boost::system::error_code& ec,
        boost::asio::ip::tcp::socket socket
    ) {
        if (ec) {
            Log("OnAsyncAccept", ec);
            if (onConnect_) {
                onConnect_(ec);
            }
            return;
        }
        
        auto session = 
            std::make_shared<WebSocketSession<WebSocketStream>>(
                std::move(socket),
                ctx_,
                onConnect_,
                onMessage_,
                onDisconnect_,
                onSend_);
        session->Init();
        return;
    }

    void Log(std::string_view ref, const boost::system::error_code& ec) {
        std::cout << ref << " > "
                << (ec ? "Error: " : "OK!")
                << (ec ? ec.message() : "")
                << std::endl;
    }

    std::function<void (boost::system::error_code, const std::string&)> onConnect_ {nullptr};
    std::function<std::vector<std::string> (boost::system::error_code,
                                            std::string&&,
                                            const std::string&)> onMessage_ {nullptr};
    std::function<void (boost::system::error_code, const std::string&)> onDisconnect_ {nullptr};
    std::function<void (boost::system::error_code, const std::string&)> onSend_ {nullptr};

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::asio::io_context& ioc_;
    boost::asio::ssl::context& ctx_;
};
};