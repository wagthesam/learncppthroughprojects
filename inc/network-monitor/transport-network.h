#pragma once

#include "network-monitor/transport-network-defs.h"

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

struct StationNode;

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

struct Step {
    Id startStationId {};
    Id endStationId {};
    Id lineId {};
    Id routeId {};
    unsigned int travelTime {0};

    bool operator==(const Step& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Step& step) {
        os << "************" << "\n"
            << "Start Station ID: " << step.startStationId << "\n"
           << "End Station ID: " << step.endStationId << "\n"
           << "Line ID: " << step.lineId << "\n"
           << "Route ID: " << step.routeId << "\n"
           << "Travel Time: " << step.travelTime << "\n"
            << "************" << "\n";
        return os;
    }
};

struct TravelRoute {

    Id startStationId {};
    Id endStationId {};
    unsigned int totalTravelTime {0};
    std::vector<Step> steps {};

    bool operator==(const TravelRoute& other) const;

    friend std::ostream& operator<<(std::ostream& os, const TravelRoute& travelRoute) {
        os << "************" << "\n"
            << "Start Station ID: " << travelRoute.startStationId << "\n"
           << "End Station ID: " << travelRoute.endStationId << "\n"
           << "Total Travel Time: " << travelRoute.totalTravelTime << "\n"
           << "Steps:\n";
        for (const auto& step : travelRoute.steps) {
            os << step << "\n";
        }
        os << "************" << "\n";   
        return os;
    }
};

struct GraphStop {
    Id stationId {};
    std::optional<Id> routeId {};
    std::optional<Id> lineId {};

    // Define equality operator
    bool operator==(const GraphStop& other) const {
        return stationId == other.stationId &&
               routeId == other.routeId &&
               lineId == other.lineId;
    }
};

struct GraphStopHash {
    size_t operator()(const GraphStop& gs) const {
        size_t h1 = std::hash<Id>{}(gs.stationId);
        size_t h2 = gs.routeId ? std::hash<Id>{}(*gs.routeId) : 0;
        size_t h3 = gs.lineId ? std::hash<Id>{}(*gs.lineId) : 0;
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Custom equality function
struct GraphStopEqual {
    bool operator()(const GraphStop& lhs, const GraphStop& rhs) const {
        return lhs.stationId == rhs.stationId &&
               lhs.routeId == rhs.routeId &&
               lhs.lineId == rhs.lineId;
    }
};

struct GraphStopDistance {
    GraphStop graphStop {};
    unsigned int distance {};

    bool operator>(const GraphStopDistance  & b) const {
        return b.distance>distance;
    }

    bool operator<(const GraphStopDistance  & b) const {
        return b.distance>distance;
    }
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

    static std::optional<Type> ToType(const std::string& type) {
        if (type == "in") {
            return Type::In;
        } else if (type == "out") {
            return Type::Out;
        } else {
            return {};
        }
    }
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

    TravelRoute GetFastestTravelRoute(
        const Id& stationA,
        const Id& stationB) const;
    private:
        StationNode* GetStationNode(const Id& stationId);

        const StationNode* GetStationNode(const Id& stationId) const;

        unsigned int GetTravelTimeDirectional(
            const Id& stationA,
            const Id& stationB
        ) const;

        bool SetTravelTimeDirectional(
            const Id& stationA,
            const Id& stationB,
            const unsigned int travelTime);

        std::unordered_map<Id, std::shared_ptr<StationNode>> stationIdToNode_;

        unsigned int penalty_ = 5;
};

} // namespace NetworkMonitor