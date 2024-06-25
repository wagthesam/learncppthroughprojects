#pragma once

#include "network-monitor/transport-network.h"
#include "network-monitor/file-downloader.h"

#include <boost/test/unit_test.hpp>

#include <utility>
#include <fstream>

namespace NetworkMonitor {

void from_json(const nlohmann::json& j, Step& s) {
    s.startStationId = j.at("startStationId").get<Id>();
    s.endStationId = j.at("endStationId").get<Id>();
    s.lineId = j.at("lineId").get<Id>();
    s.routeId = j.at("routeId").get<Id>();
    s.travelTime = j.at("travelTime").get<unsigned int>();
}

void from_json(const nlohmann::json& j, TravelRoute& tr) {
    tr.startStationId = j.at("startStationId").get<Id>();
    tr.endStationId = j.at("endStationId").get<Id>();
    tr.totalTravelTime = j.at("totalTravelTime").get<unsigned int>();
    const auto& steps = j.at("steps");
    tr.steps.clear();
    for (const auto& step : steps) {
        tr.steps.push_back(step.get<Step>());
    }
}

std::pair<TransportNetwork, TravelRoute> GetTestNetwork(
    const std::string& filenameExt,
    bool useOriginalNetworkLayoutFile = false
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