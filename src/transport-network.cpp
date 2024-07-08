#include "network-monitor/transport-network-defs.h"
#include "network-monitor/transport-network.h"
#include "network-monitor-internal/transport-network-internal.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <limits>
#include <functional>

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

bool TravelRoute::operator==(const TravelRoute& other) const {
    bool equal = startStationId == other.startStationId
        && endStationId == other.endStationId
        && totalTravelTime == other.totalTravelTime
        && steps.size() == other.steps.size();
    for (size_t i = 0; i < steps.size(); i++) {
        if (!(steps[i] == other.steps[i])) {
            equal = false;
        }
    }
    return equal;
}

bool Step::operator==(const Step& other) const {
    return startStationId == other.startStationId
        && endStationId == other.endStationId
        && lineId == other.lineId
        && routeId == other.routeId
        && travelTime == other.travelTime;
}

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
    bool success;
    for (auto it = src["stations"].begin(); it != src["stations"].end(); it++) {
        auto stationId = (*it)["station_id"].template get<std::string>();
        auto name = (*it)["name"].template get<std::string>();
        success = AddStation(Station{stationId, name});
        if (!success) {
            throw std::runtime_error("Unable to add station: " + stationId);
        }
    }

    std::unordered_map<std::string, std::string> lineIdToName;
    std::unordered_map<std::string, std::vector<Route>> lineToRoutes;
    for (auto it = src["lines"].begin(); it != src["lines"].end(); it++) {
        auto lineId = (*it)["line_id"].template get<std::string>();
        if (lineIdToName.find(lineId) == lineIdToName.end()) {
            lineIdToName[lineId] = (*it)["name"].template get<std::string>();
        }
        for (auto routesIt = (*it)["routes"].begin(); routesIt != (*it)["routes"].end(); routesIt++) {
            std::vector<Id> stops;
            for (auto stopsIt = (*routesIt)["route_stops"].begin(); stopsIt != (*routesIt)["route_stops"].end(); stopsIt++) {
                stops.emplace_back(*stopsIt);
            }
            auto route = Route{
                (*routesIt)["route_id"].template get<std::string>(),
                (*routesIt)["direction"].template get<std::string>(),
                lineId,
                (*routesIt)["start_station_id"].template get<std::string>(),
                (*routesIt)["end_station_id"].template get<std::string>(),
                stops
            };
            auto lineAndRouteIt = lineToRoutes.find(lineId);
            if (lineAndRouteIt == lineToRoutes.end()) {
                lineToRoutes[lineId] = {route};
            } else {
                lineToRoutes[lineId].push_back(route);
            }
        }
    }

    for (const auto& [lineId, route] : lineToRoutes) {
        success = AddLine(Line{
            lineId,
            lineIdToName[lineId],
            lineToRoutes[lineId]
        });
        if (!success) {
            throw std::runtime_error("Unable to add line: " + lineId);
        }
    }

    success = true;
    for (auto it = src["travel_times"].begin(); it != src["travel_times"].end(); it++) {
        success |= SetTravelTime(
            (*it)["start_station_id"].template get<std::string>(),
            (*it)["end_station_id"].template get<std::string>(),
            (*it)["travel_time"]);
    }

    return success;
}

TravelRoute TransportNetwork::GetOptimalTravelRoute(
        const Id& stationA,
        const Id& stationB,
        bool useDistance) const {
    TravelRoute route;
    route.startStationId = stationA;
    route.endStationId = stationB;
    if (stationA == stationB) {
        route.steps = {{
            stationA,
            stationA,
            {},
            {},
            0u
        }};
        return route;
    }
    std::unordered_map<GraphStop, unsigned int, GraphStopHash, GraphStopEqual> metricFromA;
    std::unordered_map<GraphStop, unsigned int, GraphStopHash, GraphStopEqual> distanceFromA;
    std::unordered_map<GraphStop, GraphStop, GraphStopHash, GraphStopEqual> stationIdToParent;
    metricFromA[{stationA, {}, {}}] = 0;
    distanceFromA[{stationA, {}, {}}] = 0;
    std::priority_queue<
        GraphStopMetric,
        std::deque<GraphStopMetric>,
        std::greater<GraphStopMetric>> nodesToVisit;
    nodesToVisit.push({{stationA, {}, {}}, 0});

    while (!nodesToVisit.empty()) {
        const auto currentStop = nodesToVisit.top().graphStop;
        const auto metric = nodesToVisit.top().metric;
        nodesToVisit.pop();

        // get neighbors
        auto stationNode = GetStationNode(currentStop.stationId);
        for (const auto& [neighborId, routesMetadata] : stationNode->GetStationIdToRoutesMetadata()) {
            // get routes
            for (const auto& routeMetadata : routesMetadata) {
                const GraphStop neighborStop {neighborId, routeMetadata.routeId, routeMetadata.lineId};
                unsigned int neighborMetric = metric + (useDistance ? routeMetadata.travelTime : GetPassengerCount(neighborId));
                unsigned int neighborDistance = distanceFromA[currentStop] + routeMetadata.travelTime;
                if (currentStop.routeId.has_value()
                    && currentStop.routeId.value() != routeMetadata.routeId
                    && currentStop.lineId.value() != routeMetadata.lineId) {
                    neighborMetric += useDistance ? penalty_ : GetPassengerCount(neighborId);
                    neighborDistance += penalty_;
                }
                if (metricFromA.find(neighborStop) == metricFromA.end() 
                    || neighborMetric < metricFromA[neighborStop]) {
                    stationIdToParent[neighborStop] = currentStop;
                    metricFromA[neighborStop] = neighborMetric;
                    nodesToVisit.push({neighborStop, neighborMetric});
                    distanceFromA[neighborStop] = neighborDistance;
                }
            }
        }
    }

    std::vector<GraphStopMetric> pathsToB {};
    for (const auto& [stop, metric]: metricFromA) {
        if (stop.stationId == stationB) {
            pathsToB.push_back({stop, metric});
        }
    }

    if (pathsToB.empty()) {
        return route;
    }

    std::sort(pathsToB.begin(), pathsToB.end(), std::less<GraphStopMetric>());

    for (auto currentStop = pathsToB[0].graphStop;
            currentStop.stationId != stationA;
            currentStop = stationIdToParent[currentStop]) {
        route.steps.push_back({
            stationIdToParent[currentStop].stationId,
            currentStop.stationId,
            currentStop.lineId.value(),
            currentStop.routeId.value(),
            distanceFromA[currentStop]-distanceFromA[stationIdToParent[currentStop]]
        });
    }
    route.totalTravelTime = distanceFromA[pathsToB[0].graphStop];
    std::reverse(route.steps.begin(), route.steps.end());
    return route;
}

TravelRoute TransportNetwork::GetFastestTravelRoute(
        const Id& stationA,
        const Id& stationB) const {
    return GetOptimalTravelRoute(stationA, stationB, true);
}

TravelRoute TransportNetwork::GetQuietTravelRoute(
        const Id& stationA,
        const Id& stationB) const {
    auto fastestRoute = GetOptimalTravelRoute(stationA, stationB, true);
    auto quietestRoute = GetOptimalTravelRoute(stationA, stationB, false);
    return fastestRoute.totalTravelTime * 1.2 > static_cast<double>(quietestRoute.totalTravelTime)
        ? quietestRoute 
        : fastestRoute;
}

} // namespace NetworkMonitor