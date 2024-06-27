#pragma once

#include "network-monitor/transport-network.h"
#include "network-monitor/file-downloader.h"

#include <boost/test/unit_test.hpp>

#include <utility>
#include <fstream>


namespace NetworkMonitor {

void from_json(const nlohmann::json& j, Step& s) {
    s.startStationId = j.at("start_station_id").get<Id>();
    s.endStationId = j.at("end_station_id").get<Id>();
    s.lineId = j.at("line_id").get<Id>();
    s.routeId = j.at("route_id").get<Id>();
    s.travelTime = j.at("travel_time").get<unsigned int>();
}

void from_json(const nlohmann::json& j, TravelRoute& tr) {
    tr.startStationId = j.at("start_station_id").get<Id>();
    tr.endStationId = j.at("end_station_id").get<Id>();
    tr.totalTravelTime = j.at("total_travel_time").get<unsigned int>();
    j.at("steps").get_to(tr.steps);
}

std::pair<TransportNetwork, TravelRoute> GetTestNetwork(
    const std::string& filenameExt,
    bool useOriginalNetworkLayoutFile = false,
    bool hasResultsFile = true
) {
    auto file = useOriginalNetworkLayoutFile 
        ? std::filesystem::path {TESTS_NETWORK_LAYOUT_JSON}
        : std::filesystem::path {TESTS_NETWORK_FILES} / (filenameExt + ".json");
    auto json = ParseJsonFile(file);
    BOOST_REQUIRE(json != nlohmann::json::object());
    TransportNetwork network;
    auto ok = network.FromJson(std::move(json));
    BOOST_REQUIRE(ok);

    auto routesFile = std::filesystem::path {TESTS_NETWORK_FILES} / (filenameExt + ".result.json");
    auto routesJson = ParseJsonFile(routesFile);
    BOOST_REQUIRE(routesJson != nlohmann::json::object());
    TravelRoute travelRoute;
    if (!hasResultsFile) {
        return {
            std::move(network),
            std::move(travelRoute)
        };
    }
    try {
        travelRoute = routesJson.get<TravelRoute>();
    } catch (...) {
        BOOST_FAIL(std::string("Could not parse: ") + routesFile.string());
    }

    return {
        std::move(network),
        std::move(travelRoute)
    };
}

}