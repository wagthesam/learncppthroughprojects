#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <iostream>

namespace NetworkMonitor {

/*! \brief Client to connect to a WebSocket server over TLS.
 *
 *  \tparam Resolver        The class to resolve the URL to an IP address. It
 *                          must support the same interface of
 *                          boost::asio::ip::tcp::resolver.
 *  \tparam WebSocketStream The WebSocket stream class. It must support the
 *                          same interface of boost::beast::websocket::stream.
 */
template <
    typename Resolver,
    typename WebSocketStream
>
class WebSocketClient {
public:
    /*! \brief Construct a WebSocket client.
     *
     *  \note This constructor does not initiate a connection.
     *
     *  \param url      The URL of the server.
     *  \param endpoint The endpoint on the server to connect to.
     *                  Example: ltnm.learncppthroughprojects.com/<endpoint>
     *  \param port     The port on the server.
     *  \param ioc      The io_context object. The user takes care of calling
     *                  ioc.run().
     */
    WebSocketClient(
        std::string_view url,
        std::string_view endpoint,
        std::string_view port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx
    ) : url_(url),
        endpoint_(endpoint),
        port_(port),
        resolver_(boost::asio::make_strand(ioc)),
        ws_(WebSocketStream(boost::asio::make_strand(ioc), ctx)) {
    }

    /*! \brief Destructor.
     */
    ~WebSocketClient() {
        if (ws_.is_open()) {
                Log("WebSocketClient being destroyed without closing ws. Closing ws...");
            Close([this](auto ec){
                Log("ws closed");
            });
        }
    }

    /*! \brief Connect to the server.
     *
     *  \param onConnect     Called when the connection fails or succeeds.
     *  \param onMessage     Called only when a message is successfully
     *                       received. The message is an rvalue reference;
     *                       ownership is passed to the receiver.
     *  \param onDisconnect  Called when the connection is closed by the server
     *                       or due to a connection error.
     */
    void Connect(
        std::function<void (boost::system::error_code)> onConnect = nullptr,
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage = nullptr,
        std::function<void (boost::system::error_code)> onDisconnect = nullptr
    ) {
        onConnect_ = onConnect;
        onMessage_ = onMessage;
        onDisconnect_ = onDisconnect;
        boost::system::error_code ec {};
        closed_ = false;
        resolver_.async_resolve(
            url_,
            port_,
            [this](auto ec, auto&& results) {
                OnResolve(ec, std::forward<decltype(results)>(results));
            });
    }

    /*! \brief Send a text message to the WebSocket server.
     *
     *  \param message The message to send. The caller must ensure that this
     *                 string stays in scope until the onSend handler is called.
     *  \param onSend  Called when a message is sent successfully or if it
     *                 failed to send.
     */
    void Send(
        std::string_view message,
        std::function<void (boost::system::error_code)> onSend = nullptr
    ) {
        ws_.async_write(boost::asio::buffer(message), [this, onSend](auto ec, auto bytes_transferred){
            if (ec) {
                Log("Send", ec);
                return;
            }
            onSend(ec);
        });
    }

    /*! \brief Close the WebSocket connection.
     *
     *  \param onClose Called when the connection is closed, successfully or
     *                 not.
     */
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
    void Log(std::string_view ref, const boost::system::error_code& ec) {
        std::cout << ref << " > "
                << (ec ? "Error: " : "OK!")
                << (ec ? ec.message() : "")
                << std::endl;
    }

    void Log(std::string_view msg) {
        std::cout << msg << std::endl;
    }

    void OnResolve(
        const boost::system::error_code& ec,
        boost::asio::ip::tcp::resolver::results_type results) {
        if (ec) {
            Log("OnResolve", ec);
            if (onConnect_) {
                onConnect_(ec);
            }
            return;
        }

        boost::beast::get_lowest_layer(ws_).async_connect(
            *results,
            [this](auto&& ec) {
                OnConnect(std::forward<decltype(ec)>(ec));
            });
    }

    void OnConnect(const boost::system::error_code& ec) {
        if (ec) {
            Log("OnConnect", ec);
            if (onConnect_) {
                onConnect_(ec);
            }
            return;
        }
        boost::beast::get_lowest_layer(ws_).expires_never();
        ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::client
        ));
        ws_.next_layer().async_handshake(
            boost::asio::ssl::stream_base::client,
            [this](auto ec) {
                OnTlsHandshake(ec);
            });
    }

    void OnHandshake(const boost::system::error_code& ec) {
        if (ec) {
            Log("OnHandshake", ec);
            onConnect_(ec);
            return;
        }
        ws_.text(true);
        if (onConnect_) {
            onConnect_(ec);
        }
        ListenToIncomingMessage(ec);
    }

    void ListenToIncomingMessage(
        const boost::system::error_code& ec
    ) {
        if (ec == boost::asio::error::operation_aborted) {
            if (onDisconnect_ && !closed_) {
                onDisconnect_(ec);
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

    void OnTlsHandshake(const boost::system::error_code& ec) {
        if (ec) {
            Log("OnTlsHandshake", ec);
            if (onConnect_) {
                onConnect_(ec);
            }
            return;
        }
        ws_.async_handshake(
            url_,
            endpoint_,
            [this](auto ec) {
                OnHandshake(ec);
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
            onMessage_(ec, boost::beast::buffers_to_string(buffer_.data()));
        }
        buffer_.clear();
    }

    const std::string url_;
    const std::string endpoint_;
    const std::string port_;
    Resolver resolver_;
    WebSocketStream ws_;

    bool closed_ {false};
    
    boost::beast::flat_buffer buffer_;

    std::function<void (boost::system::error_code)> onConnect_ {nullptr};
    std::function<void (boost::system::error_code,
                            std::string&&)> onMessage_ {nullptr};
    std::function<void (boost::system::error_code)> onDisconnect_ {nullptr};
};

using BoostWebSocketClient = WebSocketClient<
    boost::asio::ip::tcp::resolver,
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>
    >
>;

} // namespace NetworkMonitor
