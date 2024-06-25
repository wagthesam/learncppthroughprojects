#include "network-monitor-internal/transport-network-internal.h"

namespace NetworkMonitor {
    bool RouteEdge::AddRoute(const Id& routeId, const Id& lineId) {
        if (HasRoute(lineId, routeId)) {
            return false;
        }
        auto routeIdsIt = lineToRouteIds_.find(lineId);
        if (routeIdsIt == lineToRouteIds_.end()) {
            lineToRouteIds_[lineId] = {routeId};
            return true;
        }
        routeIdsIt->second.emplace_back(routeId);
        return true;
    }

    std::vector<RouteMetadata> RouteEdge::GetRouteMetadata() const {
        std::vector<RouteMetadata> routeMetadatas;
        for (auto& [lineId, routeIds] : lineToRouteIds_) {
            for (auto& routeId : routeIds) {
                routeMetadatas.emplace_back(
                    lineId,
                    routeId,
                    travelTime_
                );
            }
        }
        return routeMetadatas;
    }

    bool RouteEdge::HasRoute(const Id& lineId, const Id& routeId) const {
        auto routes = GetRoutes(lineId);
        return std::find(routes.begin(), routes.end(), routeId) != routes.end();
    }

    std::vector<Id> RouteEdge::GetRoutes(const Id& lineId) const {
        const auto& routeIdsIt = lineToRouteIds_.find(lineId);
        if (routeIdsIt == lineToRouteIds_.end()) {
            return {};
        }
        return routeIdsIt->second;
    }

    std::vector<Id> RouteEdge::GetRoutes() const {
            std::vector<Id> routes;
        for (const auto& [_, routeIds] : lineToRouteIds_) {
            routes.insert(routes.end(), routeIds.begin(), routeIds.end());
        }
        return routes;
    }

    std::shared_ptr<RouteEdge> StationNode::GetOrMakeEdge(const Id& endStationId) {
        auto stationIdEdgeIt = toStationIdToEdge_.find(endStationId);
        if (stationIdEdgeIt != toStationIdToEdge_.end()) {
            return stationIdEdgeIt->second;
        }
        auto edge = std::make_shared<RouteEdge>();
        toStationIdToEdge_[endStationId] = edge;
        return edge;
    }

    bool StationNode::AddIncomingEdge(const Id& fromStationId, std::shared_ptr<RouteEdge> edge) {
        auto stationIdEdgeIt = fromStationIdToEdge_.find(fromStationId);
        if (stationIdEdgeIt != fromStationIdToEdge_.end() && stationIdEdgeIt->second != edge) {
            return false;
        }
        fromStationIdToEdge_[fromStationId] = edge;
        return true;
    }

    RouteEdge* StationNode::GetEdge(const Id& stationId) {
        auto stationIdEdgeIt = toStationIdToEdge_.find(stationId);
        if (stationIdEdgeIt == toStationIdToEdge_.end()) {
            return nullptr;
        }
        return stationIdEdgeIt->second.get();
    }

    const RouteEdge* StationNode::GetEdge(const Id& stationId) const {
        return const_cast<StationNode*>(this)->GetEdge(stationId);
    }

    std::vector<Id> StationNode::GetRoutes() const {
        std::unordered_set<Id> routes;
        for (const auto& stationIdAndEdge : toStationIdToEdge_) {
            for (const auto& routeId : stationIdAndEdge.second->GetRoutes()) {
                routes.emplace(routeId);
            }
        }
        for (const auto& stationIdAndEdge : fromStationIdToEdge_) {
            for (const auto& routeId : stationIdAndEdge.second->GetRoutes()) {
                routes.emplace(routeId);
            }
        }
        return {routes.begin(), routes.end()};
    }

    std::unordered_map<Id, std::vector<RouteMetadata>> StationNode::GetStationIdToRoutesMetadata() const {
        std::unordered_map<Id, std::vector<RouteMetadata>> metadataMap;
        for (auto& [stationId, edgePtr] : toStationIdToEdge_) {
            metadataMap[stationId] = edgePtr->GetRouteMetadata();
        }
        return metadataMap;
    }
}
