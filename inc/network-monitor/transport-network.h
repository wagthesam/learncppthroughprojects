#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace {
    void Log(const std::string& msg, const std::string& caller) {
        std::cerr << caller << ": " << msg << std::endl;
    }
}

namespace NetworkMonitor {

/*! \brief A station, line, or route ID.
 */
using Id = std::string;

/*! \brief Network station
 *
 *  A Station struct is well formed if:
 *  - `id` is unique across all stations in the network.
 */
struct Station {
    Id id {};
    std::string name {};

    /*! \brief Station comparison
     *
     *  Two stations are "equal" if they have the same ID.
     */
    bool operator==(const Station& other) const;
    bool operator!=(const Station& other) const;
};

/*! \brief Network route
 *
 *  Each underground line has one or more routes. A route represents a single
 *  possible journey across a set of stops in a specified direction.
 *
 *  There may or may not be a corresponding route in the opposite direction of
 *  travel.
 *
 *  A Route struct is well formed if:
 *  - `id` is unique across all lines and their routes in the network.
 *  - The `lineId` line exists and has this route among its routes.
 *  - `stops` has at least 2 stops.
 *  - `startStationId` is the first stop in `stops`.
 *  - `endStationId` is the last stop in `stops`.
 *  - Every `stationId` station in `stops` exists.
 *  - Every stop in `stops` appears only once.
 */
struct Route {
    Id id {};
    std::string direction {};
    Id lineId {};
    Id startStationId {};
    Id endStationId {};
    std::vector<Id> stops {};

    /*! \brief Route comparison
     *
     *  Two routes are "equal" if they have the same ID.
     */
    bool operator==(const Route& other) const;
    bool operator!=(const Route& other) const;
};

/*! \brief Network line
 *
 *  A line is a collection of routes serving multiple stations.
 *
 *  A Line struct is well formed if:
 *  - `id` is unique across all lines in the network.
 *  - `routes` has at least 1 route.
 *  - Every route in `routes` is well formed.
 *  - Every route in `routes` has a `lineId` that is equal to this line `id`.
 */
struct Line {
    Id id {};
    std::string name {};
    std::vector<Route> routes {};

    /*! \brief Line comparison
     *
     *  Two lines are "equal" if they have the same ID.
     */
    bool operator==(const Line& other) const;
    bool operator!=(const Line& other) const;
};

/*! \brief Passenger event
 */
struct PassengerEvent {
    enum class Type {
        In,
        Out
    };

    Id stationId {};
    Type type {Type::In};
};

/*! \brief Underground network representation
 */
class TransportNetwork {
public:
    /*! \brief Default constructor
     */
    TransportNetwork() = default;

    /*! \brief Destructor
     */
    ~TransportNetwork() = default;

    /*! \brief Copy constructor
     */
    TransportNetwork(
        const TransportNetwork& copied
    )  = default;

    /*! \brief Move constructor
     */
    TransportNetwork(
        TransportNetwork&& moved
    ) = default;

    /*! \brief Copy assignment operator
     */
    TransportNetwork& operator=(
        const TransportNetwork& copied
    ) = default;

    /*! \brief Move assignment operator
     */
    TransportNetwork& operator=(
        TransportNetwork&& moved
    ) = default;

    /*! \brief Add a station to the network.
     *
     *  \returns false if there was an error while adding the station to the
     *           network.
     *
     *  This function assumes that the Station object is well-formed.
     *
     *  The station cannot already be in the network.
     */
    bool AddStation(
        const Station& station
    );

    /*! \brief Add a line to the network.
     *
     *  \returns false if there was an error while adding the station to the
     *           network.
     *
     *  This function assumes that the Line object is well-formed.
     *
     *  All stations served by this line must already be in the network. The
     *  line cannot already be in the network.
     */
    bool AddLine(
        const Line& line
    );

    /*! \brief Record a passenger event at a station.
     *
     *  \returns false if the station is not in the network or if the passenger
     *           event is not reconized.
     */
    bool RecordPassengerEvent(
        const PassengerEvent& event
    );

    /*! \brief Get the number of passengers currently recorded at a station.
     *
     *  The returned number can be negative: This happens if we start recording
     *  in the middle of the day and we record more exiting than entering
     *  passengers.
     *
     *  \throws std::runtime_error if the station is not in the network.
     */
    long long int GetPassengerCount(
        const Id& station
    ) const;

    /*! \brief Get list of routes serving a given station.
     *
     *  \returns An empty vector if there was an error getting the list of
     *           routes serving the station, or if the station has legitimately
     *           no routes serving it.
     *
     *  The station must already be in the network.
     */
    std::vector<Id> GetRoutesServingStation(
        const Id& station
    ) const;

    /*! \brief Set the travel time between 2 adjacent stations.
     *
     *  \returns false if there was an error while setting the travel time
     *           between the two stations.
     *
     *  The travel time is the same for all routes connecting the two stations
     *  directly.
     *
     *  The two stations must be adjacent in at least one line route. The two
     *  stations must already be in the network.
     */
    bool SetTravelTime(
        const Id& stationA,
        const Id& stationB,
        const unsigned int travelTime
    );

    /*! \brief Get the travel time between 2 adjacent stations.
     *
     *  \returns 0 if the function could not find the travel time between the
     *           two stations, or if station A and B are the same station.
     *
     *  The travel time is the same for all routes connecting the two stations
     *  directly.
     *
     *  The two stations must be adjacent in at least one line route. The two
     *  stations must already be in the network.
     */
    unsigned int GetTravelTime(
        const Id& stationA,
        const Id& stationB
    ) const;

    /*! \brief Get the total travel time between any 2 stations, on a specific
     *         route.
     *
     *  The total travel time is the cumulative sum of the travel times between
     *  all stations between `stationA` and `stationB`.
     *
     *  \returns 0 if the function could not find the travel time between the
     *           two stations, or if station A and B are the same station.
     *
     *  The two stations must be both served by the `route`. The two stations
     *  must already be in the network.
     */
    unsigned int GetTravelTime(
        const Id& line,
        const Id& route,
        const Id& stationA,
        const Id& stationB
    ) const;

    /*! \brief Populate the network from a JSON object.
     *
     *  \param src Ownership of the source JSON object is moved to this method.
     *
     *  \returns false if stations and lines where parsed successfully, but not
     *           the travel times.
     *
     *  \throws std::runtime_error This method throws if the JSON items were
     *                             parsed correctly but there was an issue
     *                             adding new stations or lines to the network.
     *  \throws nlohmann::json::exception If there was a problem parsing the
     *                                    JSON object.
     */
    bool FromJson(
        nlohmann::json&& srcs
    );
    private:
        class RouteEdge {
        public:
            RouteEdge() : travelTime_(0) {}

            bool AddRoute(const Id& routeId, const Id& lineId) {
                if (HasRoute(lineId, routeId)) {
                    Log("Route exists: " + routeId, "AddRoute");
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

            bool HasRoute(const Id& lineId, const Id& routeId) const {
                auto routes = GetRoutes(lineId);
                return std::find(routes.begin(), routes.end(), routeId) != routes.end();
            }

            std::vector<Id> GetRoutes(const Id& lineId) const {
                std::vector<Id> routes;
                const auto& routeIdsIt = lineToRouteIds_.find(lineId);
                if (routeIdsIt == lineToRouteIds_.end()) {
                    return routes;
                }
                return routeIdsIt->second;
            }

            std::vector<Id> GetRoutes() const {
                std::vector<Id> routes;
                for (const auto& lineAndRouteId : lineToRouteIds_) {
                    for (const auto& route : lineAndRouteId.second) {
                        routes.emplace_back(route);
                    }
                }
                return routes;
            }

            unsigned int GetTravelTime() const {
                return travelTime_;
            }

            void SetTravelTime(unsigned int travelTime) {
                travelTime_ = travelTime;
            }
        private:
            unsigned int travelTime_;
            std::unordered_map<Id, std::vector<Id>> lineToRouteIds_;
        };

        class StationNode {
        public:
            StationNode() = default;

            bool AddEdge(std::shared_ptr<RouteEdge> edge, const Id& endStationId) {
                auto edgePt = GetEdge(endStationId);
                if (edgePt != nullptr) {
                    Log("Failed to add edge to:" + endStationId, "AddEdge");
                    return false;
                }
                toStationIdToEdge_[endStationId] = edge;
                return true;
            }

            bool AddRoute(const Id& routeId, const Id& lineId, const Id& endStationId) {
                auto edgePt = GetEdge(endStationId);
                if (edgePt == nullptr) {
                    toStationIdToEdge_[endStationId] = std::make_shared<RouteEdge>();
                    edgePt = toStationIdToEdge_[endStationId].get();
                }
                bool success = edgePt->AddRoute(routeId, lineId);
                if (!success) {
                    Log("Failed to add route:" + routeId, "AddRoute");
                    return false;
                }
                return true;
            }

            bool AddIncomingRoute(const Id& fromStationId, std::shared_ptr<RouteEdge> edge) {
                auto stationIdEdgeIt = fromStationIdToEdge_.find(fromStationId);
                if (stationIdEdgeIt != fromStationIdToEdge_.end() && stationIdEdgeIt->second != edge) {
                    Log("Conflicting edges", "AddIncomingRoute");
                    return false;
                }
                fromStationIdToEdge_[fromStationId] = edge;
                return true;
            }

            std::shared_ptr<RouteEdge> GetEdgeSharedPtr(const Id& stationId) {
                auto stationIdEdgeIt = toStationIdToEdge_.find(stationId);
                if (stationIdEdgeIt == toStationIdToEdge_.end()) {
                    Log("Could not find edge to station Id: " + stationId, "GetEdge");
                    return nullptr;
                }
                return stationIdEdgeIt->second;
            }

            RouteEdge* GetEdge(const Id& stationId) {
                auto stationIdEdgeIt = toStationIdToEdge_.find(stationId);
                if (stationIdEdgeIt == toStationIdToEdge_.end()) {
                    Log("Could not find edge to station Id: " + stationId, "GetEdge");
                    return nullptr;
                }
                return stationIdEdgeIt->second.get();
            }

            const RouteEdge* GetEdge(const Id& stationId) const {
                return const_cast<StationNode*>(this)->GetEdge(stationId);
            }

            std::vector<Id> GetRoutes() const {
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

            void AddPassenger() {
                passengers_++;
            }

            void RemovePassenger() {
                passengers_--;
            }

            int GetPassengers() const {
                return passengers_;
            }

            std::optional<std::pair<Id, unsigned int>> GetNextStationAndTime(
                const Id& line,
                const Id& route) const {
                for (auto stationIdAndNode : toStationIdToEdge_) {
                    if (stationIdAndNode.second->HasRoute(line, route)) {
                        return std::pair<Id, unsigned int>(
                            stationIdAndNode.first, 
                            stationIdAndNode.second->GetTravelTime());
                    }
                }
                return {};
            }
        private:
            int passengers_;
            std::unordered_map<Id, std::shared_ptr<RouteEdge>> toStationIdToEdge_;
            std::unordered_map<Id, std::shared_ptr<RouteEdge>> fromStationIdToEdge_;
        };
        StationNode* GetStationNode(const Id& stationId) {
            auto stationResult = stationIdToNode_.find(stationId);
            if (stationResult == stationIdToNode_.end()) {
                return nullptr;
            }
            return &stationResult->second;
        }

        const StationNode* GetStationNode(const Id& stationId) const {
            return const_cast<TransportNetwork*>(this)->GetStationNode(stationId);
        }

        unsigned int GetTravelTimeDirectional(
            const Id& stationA,
            const Id& stationB
        ) const;

        std::unordered_map<Id, StationNode> stationIdToNode_;
};

} // namespace NetworkMonitor