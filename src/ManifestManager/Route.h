#ifndef ROUTE_H
#define ROUTE_H

#include <string>
#include <vector>

class Route{
    private:
        std::string callsign;
        std::string code;
        std::string number;
        std::string airlinecoode;
        std::vector<std::string> waypoints;

    public:
        Route() = default;
        ~Route() = default;
        
        Route(const std::string& cs, const std::string& c, const std::string& n, const std::string& ac, const std::vector<std::string>& wp);
        Route(const Route& other);
        Route& operator=(const Route& other);
        
        std::string getCallsign() const;
        std::string getCode() const;
        std::string getNumber() const;
        std::string getAirlineCode() const;
        std::vector<std::string> getWaypoints() const;
};
#endif