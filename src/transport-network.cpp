#include "network-monitor/transport-network-defs.h"
#include "network-monitor/transport-network.h"
#include "network-monitor-internal/transport-network-internal.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

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
        return false;
    }
    stationIdToNode_[station.id] = std::make_shared<StationNode>();
    return true;
}

bool TransportNetwork::AddLine(const Line& line) {
    for (const auto& route : line.routes) {
        for (size_t idx = 1; idx < route.stops.size(); idx++) {
            const auto& prevStationId = route.stops[idx-1];
            const auto& curStationId = route.stops[idx];

            auto nodePt = GetStationNode(prevStationId);
            if (nodePt == nullptr) {
                return false;
            }
            auto edge = nodePt->GetOrMakeEdge(curStationId);
            bool success = edge->AddRoute(
                route.id,
                route.lineId
            );
            if (!success) {
                return false;
            }

            auto curNodePt = GetStationNode(curStationId);
            if (nodePt == nullptr) {
                return false;
            }
            curNodePt->AddIncomingEdge(prevStationId, edge);
        }
    }
    return true;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event) {
    auto nodePt = GetStationNode(event.stationId);
    if (nodePt == nullptr) {
        return false;
    }
    event.type == PassengerEvent::Type::In ? nodePt->passengers_++ : nodePt->passengers_--;
    return true;
}

long long int TransportNetwork::GetPassengerCount(const Id& station) const {
    auto nodePt = GetStationNode(station);
    if (nodePt == nullptr) {
        throw std::runtime_error("Station not found: " + station);
    }
    return nodePt->passengers_;
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
    bool success = SetTravelTimeDirectional(stationA, stationB, travelTime);
    success |= SetTravelTimeDirectional(stationB, stationA, travelTime);
    return success;
}

bool TransportNetwork::SetTravelTimeDirectional(
    const Id& stationA,
    const Id& stationB,
    const unsigned int travelTime) {
    auto nodePt = GetStationNode(stationA);
    if (nodePt == nullptr) {
        return false;
    }
    auto edgePt = nodePt->GetEdge(stationB);
    bool success = false;
    if (edgePt != nullptr) {
        edgePt->travelTime_ = travelTime;
        success = true;
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
    return edgePt->travelTime_;
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
            return 0;
        }
        bool foundNextEdge = false;
        for (const auto& [toStationId, edge] : stationPt->toStationIdToEdge_) {
            if (edge->HasRoute(line, route)) {
                cur_station = toStationId;
                time += edge->travelTime_;
                foundNextEdge = true;
                break;
            }
        }
        if (!foundNextEdge) {
            return 0;
        }
    }
    return time;
}

StationNode* TransportNetwork::GetStationNode(const Id& stationId) {
    auto stationResult = stationIdToNode_.find(stationId);
    if (stationResult == stationIdToNode_.end()) {
        return nullptr;
    }
    return stationResult->second.get();
}

const StationNode* TransportNetwork::GetStationNode(const Id& stationId) const {
    return const_cast<TransportNetwork*>(this)->GetStationNode(stationId);
}

bool TransportNetwork::FromJson(
    nlohmann::json&& src) {
    return true;
}

} // namespace NetworkMonitor