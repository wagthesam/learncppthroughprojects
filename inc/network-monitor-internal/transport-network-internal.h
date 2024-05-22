#pragma once

#include "network-monitor/transport-network-defs.h"

#include <string>
#include <vector>
#include <unordered_set>

namespace NetworkMonitor {
    struct RouteEdge {
        bool AddRoute(const Id& routeId, const Id& lineId);

        bool HasRoute(const Id& lineId, const Id& routeId) const;

        std::vector<Id> GetRoutes(const Id& lineId) const;

        std::vector<Id> GetRoutes() const;

        unsigned int travelTime_;
        std::unordered_map<Id, std::vector<Id>> lineToRouteIds_;
    };

    struct StationNode {
        std::shared_ptr<RouteEdge> GetOrMakeEdge(const Id& endStationId);

        bool AddIncomingEdge(const Id& fromStationId, std::shared_ptr<RouteEdge> edge);

        RouteEdge* GetEdge(const Id& stationId);

        const RouteEdge* GetEdge(const Id& stationId) const;

        std::vector<Id> GetRoutes() const;

        int passengers_;
        std::unordered_map<Id, std::shared_ptr<RouteEdge>> toStationIdToEdge_;
        std::unordered_map<Id, std::shared_ptr<RouteEdge>> fromStationIdToEdge_;
    };
}