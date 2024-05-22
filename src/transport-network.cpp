#include "network-monitor/transport-network.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <iostream>

namespace {
    template <class T>
    bool isEqual(std::vector<T> a, std::vector<T> b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }
}

namespace NetworkMonitor {

bool Station::operator==(const Station& other) const {
    return other.id == id && other.name == name;
}

bool Route::operator==(const Route& other) const {
    return id == other.id
        && direction == other.direction
        && id == other.id
        && startStationId == other.startStationId
        && endStationId == other.endStationId
        && isEqual(stops, other.stops);
}

bool Line::operator==(const Line& other) const {
    return id == other.id
        && name == other.name
        && isEqual(routes, other.routes);
}

bool Station::operator!=(const Station& other) const {
    return !(*this == other);
}

bool Route::operator!=(const Route& other) const {
    return !(*this == other);
}

bool Line::operator!=(const Line& other) const {
    return !(*this == other);
}

bool TransportNetwork::AddStation(const Station& station) {
    auto nodePt = GetStationNode(station.id);
    if (nodePt != nullptr) {
        Log("Station already exists in network: " + station.name + ", " + station.id, "AddStation");
        return false;
    }
    stationIdToNode_[station.id] = StationNode();
    return true;
}

bool TransportNetwork::AddLine(const Line& line) {
    for (const auto& route : line.routes) {
        for (size_t idx = 1; idx < route.stops.size(); idx++) {
            const auto& prevStationId = route.stops[idx-1];
            const auto& curStationId = route.stops[idx];

            auto nodePt = GetStationNode(prevStationId);
            if (nodePt == nullptr) {
                Log("Add line failed, station not on network:" + prevStationId, "AddLine");
                return false;
            }
            auto edge = nodePt->GetEdgeSharedPtr(curStationId);
            if (edge == nullptr) {
                edge = std::make_shared<RouteEdge>();
                nodePt->AddEdge(edge, curStationId);
            }

            bool success = edge->AddRoute(
                route.id,
                route.lineId
            );
            if (!success) {
                Log("Unable to add route:" + route.id, "AddLine");
                return false;
            }

            auto curNodePt = GetStationNode(curStationId);
            if (nodePt == nullptr) {
                Log("Add line failed, station not on network:" + curStationId, "AddLine");
                return false;
            }
            curNodePt->AddIncomingRoute(prevStationId, edge);
        }
    }
    return true;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event) {
    auto nodePt = GetStationNode(event.stationId);
    if (nodePt == nullptr) {
        Log("Could not find station Id: " + event.stationId, "RecordPassengerEvent");
        return false;
    }
    event.type == PassengerEvent::Type::In ? nodePt->AddPassenger() : nodePt->RemovePassenger();
    return true;
}

long long int TransportNetwork::GetPassengerCount(const Id& station) const {
    auto nodePt = GetStationNode(station);
    if (nodePt == nullptr) {
        Log("Could not find station Id: " + station, "GetPassengerCount");
        throw std::runtime_error("Station not found");
    }
    return nodePt->GetPassengers();
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(const Id& station) const {
    auto nodePt = GetStationNode(station);
    if (nodePt == nullptr) {
        return std::vector<Id>();
    }
    return nodePt->GetRoutes();
}

bool TransportNetwork::SetTravelTime(
    const Id& stationA,
    const Id& stationB,
    const unsigned int travelTime) {
    auto nodePt = GetStationNode(stationA);
    if (nodePt == nullptr) {
        Log("Could not find station Id: " + stationA, "SetTravelTime");
        return false;
    }
    auto edgePt = nodePt->GetEdge(stationB);
    bool success = false;
    if (edgePt != nullptr) {
        edgePt->SetTravelTime(travelTime);
        success = true;
    }

    auto nodeBPt = GetStationNode(stationB);
    if (nodeBPt == nullptr) {
        Log("Could not find station Id: " + stationB, "SetTravelTime");
        return false;
    }
    auto reverseEdgePt = nodeBPt->GetEdge(stationA);
    if (reverseEdgePt != nullptr) {
        reverseEdgePt->SetTravelTime(travelTime);
        success = true;
    }
    if (!success) {
        Log("Unable to find an edge", "SetTravelTime");
    }
    return success;
}

unsigned int TransportNetwork::GetTravelTimeDirectional(
        const Id& stationA,
        const Id& stationB) const {
    if (stationA == stationB) {
        return 0;
    }
    auto nodePt = GetStationNode(stationA);
    if (nodePt == nullptr) {
        return 0;
    }
    auto edgePt = nodePt->GetEdge(stationB);
    if (edgePt == nullptr) {
        return 0;
    }
    return edgePt->GetTravelTime();
}

unsigned int TransportNetwork::GetTravelTime(
        const Id& stationA,
        const Id& stationB) const {
    return std::max(
        GetTravelTimeDirectional(stationA, stationB),
        GetTravelTimeDirectional(stationB, stationA));
}
    
unsigned int TransportNetwork::GetTravelTime(
    const Id& line,
    const Id& route,
    const Id& stationA,
    const Id& stationB) const {
    auto cur_station = stationA;
    unsigned int time = 0u;
    while (cur_station != stationB) {
        auto stationPt = GetStationNode(cur_station);
        if (stationPt == nullptr) {
            Log("Could not find station Id: " + cur_station, "GetTravelTime");
            return 0;
        }
        auto maybeStationTimePair = stationPt->GetNextStationAndTime(line, route);
        if (!maybeStationTimePair.has_value()) {
            return 0;
        }
        cur_station = maybeStationTimePair.value().first;
        time += maybeStationTimePair.value().second;
    }
    return time;
}

bool TransportNetwork::FromJson(
    nlohmann::json&& src) {
    return true;
}

} // namespace NetworkMonitor