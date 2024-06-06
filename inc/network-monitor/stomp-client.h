#pragma once

#include <network-monitor/stomp-frame.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <sstream>
#include <string>
#include <random>

using namespace std::string_literals;

namespace NetworkMonitor {

/*! \brief Error codes for the STOMP client.
 */
enum class StompClientError {
    kOk = 0,
    kTimeout = 1,
    kError = 2,
    // TODO: Your enum values go here
    // ...
};

struct SubscribeToken {
    std::string subscriptionId;
    std::string receiptId;
};

/*! \brief STOMP client implementing the subset of commands needed by the
 *         network-events service.
 *
 *  \tparam WsClient    WebSocket client class. This type must have the same
 *                      interface of WebSocketClient.
 */
template <typename WsClient>
class StompClient {
public:
    /*! \brief Construct a STOMP client connecting to a remote URL/port through
     *         a secure WebSocket connection.
     *
     *  \note This constructor does not initiate a connection.
     *
     *  \param url      The URL of the server.
     *  \param endpoint The endpoint on the server to connect to.
     *                  Example: ltnm.learncppthroughprojects.com/<endpoint>
     *  \param port     The port on the server.
     *  \param ioc      The io_context object. The user takes care of calling
     *                  ioc.run().
     *  \param ctx      The TLS context to setup a TLS socket stream.
     */
    StompClient(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx
    ) : ws_(WsClient(
            url,
            endpoint,
            port,
            ioc,
            ctx
        )), 
        endpoint_(endpoint),
        url_(url) {}

    // ...

    /*! \brief Connect to the STOMP server.
     */
    void Connect(
        const std::string& username,
        const std::string& password,
        std::function<void (StompClientError, std::string&&)> onConnect,
        std::function<void (StompClientError, std::string&&)> onDisconnect
    ) {
        onConnect_ = onConnect;
        onDisconnect_ = onDisconnect;
        ws_.Connect(
            [&username, &password, this](boost::system::error_code ec) {
                if (ec) {
                    onConnect_(StompClientError::kError, "Ws Error");
                } else {
                    OnWsConnect(username, password);
                }
            },
            [this, username, password](boost::system::error_code ec, std::string&& msg){
                if (ec) {
                    MessageHandler(StompClientError::kError, std::forward<std::string>(msg));
                } else {
                    MessageHandler(StompClientError::kOk, std::forward<std::string>(msg));
                }
            },
            [this](boost::system::error_code ec) {
                if (connected_ && !disconnected_) {
                    disconnected_ = true;
                    if (ec) {
                        onDisconnect_(StompClientError::kError, "");
                    } else {
                        onDisconnect_(StompClientError::kOk, "");
                    }
                }
            }
        );
    }

    void OnWsConnect(
        const std::string& username,
        const std::string& password
    ) {
        std::stringstream ss;
        ss << "STOMP\n"
            << "accept-version:1.2\n"
            << "host:" << url_ << "\n"
            << "login:" << username << "\n"
            << "passcode:" << password << "\n"
            << "\n"
            << "\0"s;

        ws_.Send(ss.str(), 
                [this](auto ec) {
                    if (ec) {
                        onConnect_(StompClientError::kError, "OnWsConnect: ws error");
                    }
                }
        );
    }

    /*! \brief Close the STOMP and WebSocket connection.
     */
    void Close(
         std::function<void (StompClientError)> onClose
    )
    {
        ws_.Close(
                [this, onClose](auto ec) {
                    if (ec) {
                        onClose(StompClientError::kError);
                    } else {
                        onClose(StompClientError::kOk);
                    }
                    disconnected_ = true;
                }
        );
    }

    /*! \brief Subscribe to a STOMP endpoint.
     *
     *  \returns The subscription ID.
     */
    SubscribeToken Subscribe(
        std::function<void (StompClientError, std::string&&)> onSubscribe,
        std::function<void (StompClientError, std::string&&)> onMessage
    )
    {
        if (subscribed_) {
            return {subscriptionId_, receiptId_};
        }
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<> distrib(0, 99999);
        subscriptionId_ = std::to_string(distrib(rng));
        receiptId_ = std::to_string(distrib(rng));

        onSubscribe_ = onSubscribe;
        onMessage_ = onMessage;

        std::stringstream ss;
        ss << "SUBSCRIBE\n"
            << "id:" << subscriptionId_ << "\n"
            << "receipt:" << receiptId_ << "\n"
            << "destination:/passengers\n"
            << "ack:auto\n"
            << "\n"
            << "\0"s;

        ws_.Send(ss.str(), 
                [this](auto ec) {
                    if (ec) {
                        onSubscribe_(
                            StompClientError::kError,
                            "Could not successfully send SUBSCRIBE frame.");
                    }
                }
        );
        return {subscriptionId_, receiptId_};
    }

    bool IsConnected() const {
        return connected_ && !disconnected_;
    }

    bool IsSubscribed() const {
        return subscribed_ && !disconnected_;
    }

    WsClient* GetWsClient() {
        return &ws_;
    }

    private:

    void MessageHandler(StompClientError clientError, std::string&& msg) {
        if (clientError == StompClientError::kOk) {
            StompError error;
            StompFrame frame{error, msg};
            if (error == StompError::kOk) {
                if (frame.GetCommand() == StompCommand::kConnected) {
                    connected_ = true;
                    if (onConnect_) onConnect_(StompClientError::kOk, "");
                } else if (frame.GetCommand() == StompCommand::kError) {
                    if (!connected_ && onConnect_) {
                        onConnect_(StompClientError::kError, std::forward<std::string>(msg));
                    } else if (!subscribed_ && onSubscribe_) {
                        onSubscribe_(StompClientError::kError, std::forward<std::string>(msg));
                    } else if (connected_ && subscribed_ && onMessage_) {
                        onMessage_(StompClientError::kError, std::string(frame.GetBody()));
                    }
                } else if (frame.GetCommand() == StompCommand::kReceipt) {
                    if (receiptId_ == frame.GetHeaderValue(StompHeader::kReceiptId)) {
                        subscribed_ = true;
                        if (onSubscribe_) onSubscribe_(StompClientError::kOk, "Success");
                    } else if (onSubscribe_) {
                        onSubscribe_(StompClientError::kError, "Receipt: Invalid headers -" + msg);
                    }
                } else if (frame.GetCommand() == StompCommand::kMessage) {
                    bool valid = frame.GetHeaderValue(StompHeader::kSubscription) == subscriptionId_
                        && frame.GetHeaderValue(StompHeader::kDestination) == "/passengers";
                    if (valid) {
                        onMessage_(StompClientError::kOk, std::string(frame.GetBody()));
                    } else {
                        onMessage_(StompClientError::kError, 
                            "Message: Invalid headers -" + msg);
                    }
                } else {
                    onMessage_(StompClientError::kError, "Unable to handle STOMP message");
                }
            } else if (!connected_ && onConnect_) {
                onConnect_(StompClientError::kError, "Error parsing message: " + msg);
            } else if (!subscribed_ && onSubscribe_) {
                onSubscribe_(StompClientError::kError, "Error parsing message: " + msg);
            } else if (connected_ && subscribed_ && onMessage_) {
                onMessage_(StompClientError::kError, "Error parsing message: " + msg);
            } else {
                std::cout << "Stomp error: Error parsing message: " << msg << std::endl;
            }
        } else {
            std::cout << "Error receiving message: " << msg << std::endl;
        }
    }

    WsClient ws_;

    bool connected_ {false};
    bool subscribed_ {false};
    bool disconnected_ {false};

    std::string subscriptionId_;
    std::string receiptId_;
    std::string endpoint_;
    std::string url_;

    std::function<void (StompClientError, std::string&&)> onMessage_;
    std::function<void (StompClientError, std::string&&)> onSubscribe_;
    std::function<void (StompClientError, std::string&&)> onConnect_;
    std::function<void (StompClientError, std::string&&)> onDisconnect_;
};

} // namespace NetworkMonitor
