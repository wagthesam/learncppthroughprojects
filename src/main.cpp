#include "network-monitor/network-monitor.h"
#include "network-monitor/stomp-client.h"
#include "network-monitor/websocket-client.h"

#include <boost/program_options.hpp>

#include <string>
#include <iostream>

namespace po = boost::program_options;

int  main(int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("url,u", po::value<std::string>(), "URL of the network monitoring service")
            ("endpoint,e", po::value<std::string>(), "Endpoint for network operations")
            ("port,p", po::value<std::string>(), "Port number for the network service")
            ("username", po::value<std::string>(), "Username for authentication")
            ("password", po::value<std::string>(), "Password for authentication")
            ("stomp_endpoint", po::value<std::string>(), "STOMP protocol endpoint")
            ("cert_path", po::value<std::string>(), "Path to the SSL/TLS certificate")
            ("network_layout_path", po::value<std::string>(), "Filesystem path to network layout configuration file")
            ("runtime_s", po::value<int>(), "How many seconds to run client for");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        NetworkMonitor::NetworkMonitorConfig config {
            vm["url"].as<std::string>(),
            vm["endpoint"].as<std::string>(),
            vm["port"].as<std::string>(),
            vm["username"].as<std::string>(),
            vm["password"].as<std::string>(),
            vm["stomp_endpoint"].as<std::string>(),
            vm["cert_path"].as<std::string>(),
            vm["network_layout_path"].as<std::string>()
        };
        
        NetworkMonitor::NetworkMonitor<
            NetworkMonitor::StompClient<NetworkMonitor::BoostWebSocketClient>> networkMonitor;
        networkMonitor.Configure(config);
        if (vm.count("runtime_s")) {
            networkMonitor.Run(vm["runtime_s"].as<int>());
        } else {
            networkMonitor.Run();
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error!\n";
        return 1;
    }

    return 0;
}