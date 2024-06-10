#pragma once

#include "network-monitor/transport-network.h"
#include "network-monitor/stomp-client.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>
#include <fstream>

namespace NetworkMonitor {
struct NetworkMonitorConfig {
    std::string url;
    std::string endpoint;
    std::string port;
    std::string username;
    std::string password;
    std::string stompEndpoint;
    std::string certPath;
    std::string networkLayoutPath;
};

template <class Client>
class NetworkMonitor {
public:
    NetworkMonitor() : ctx_{boost::asio::ssl::context::tlsv12_client} {};

    bool Configure(const NetworkMonitorConfig& config) {
        if (!network_.FromJson(nlohmann::json::parse(std::ifstream{config.networkLayoutPath}))) {
            return false;
        }
        ctx_.load_verify_file(config.certPath);
        client_ = std::make_unique<Client>(
            config.url,
            config.endpoint,
            config.port,
            ioc_,
            ctx_
        );
        client_->Connect(
            config.username,
            config.password,
            [this, config = std::move(config)](StompClientError error, std::string&& msg) {
                if (error == StompClientError::kOk) {
                    Log("OnConnect", "ok");
                    client_->Subscribe(
                        [this](StompClientError error, std::string&& msg) {
                            if (error == StompClientError::kOk) {
                                Log("OnSubscribe", "ok");
                            } else {
                                Log("OnSubscribe", "error: " + msg);
                            }
                        },
                        [this](StompClientError error, std::string&& msg) {
                            OnMessage(error, std::move(msg));
                        }
                    );
                } else {
                    Log("OnConnect", "error: " + msg);
                }
            },
            [this](StompClientError error, std::string&& msg) {
                if (error == StompClientError::kOk) {
                    Log("OnDisconnect", "ok");
                } else {
                    Log("OnDisconnect", "error: " + msg);
                }
            }
        );
        return true;
    }

    void Run() {
        ioc_.run();
    }

    void Run(int runtime_s) {
        boost::asio::high_resolution_timer timer(ioc_);
        timer.expires_after(std::chrono::seconds(runtime_s));
        timer.async_wait([this](auto ec) {
            client_->Close(
                [this](StompClientError error) {
                    if (error == StompClientError::kOk) {
                        Log("OnClose", "ok");
                    } else {
                        Log("OnClose", "error");
                    }
                }
            );
        });
        ioc_.run();
    }
private:
    void OnMessage(StompClientError error, std::string&& msg) {
        Log("Received:", msg);
        if (error == StompClientError::kOk) {
            auto event = nlohmann::json::parse(msg);
            auto passengerEvent = event["passenger_event"].template get<std::string>();
            auto stationId = event["station_id"].template get<std::string>();

            auto eventType = PassengerEvent::ToType(passengerEvent);
            if (eventType.has_value()) {
                PassengerEvent event {stationId, eventType.value()};
                network_.RecordPassengerEvent(event);
            } else {
                Log("OnMessage", "parse error: " + msg);
            }
        } else {
            Log("OnMessage", "receive error: " + msg);
        }
    }

    void Log(const std::string& source, const std::string& msg) const {
        std::cout << " " << source << " | " << msg << std::endl;
    }

    TransportNetwork network_;
    boost::asio::io_context ioc_;
    boost::asio::ssl::context ctx_;
    std::unique_ptr<Client> client_;
};

}
