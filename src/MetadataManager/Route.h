#ifndef ROUTE_H
#define ROUTE_H

#include <string>
#include <vector>

class Route{
    std::string callsign;
    std::string code;
    std::string number;
    std::string airlinecoode;
    std::vector<std::string> waypoints;
    std::string waypointsStr;

public:
    Route() = default;
    Route(const std::string& cs, const std::string& c, const std::string& n, const std::string& ac, const std::string& wp);
    Route(const Route& other);
    ~Route() = default;

    Route& operator=(const Route& other);
    
    const std::string& getCallsign() const;
    const std::string& getCode() const;
    const std::string& getNumber() const;
    const std::string& getAirlineCode() const;
    const std::vector<std::string>& getWaypoints() const;
    const std::string& getWaypointStr() const;

private:
    void splitStringByDash(const std::string& input, std::vector<std::string>& result) const;
};
#endif  // ROUTE_H