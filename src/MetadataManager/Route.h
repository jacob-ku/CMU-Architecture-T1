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
        std::string waypointsStr; // For storing the original waypoints string

    public:
        Route() = default;
        ~Route() = default;
        
        Route(const std::string& cs, const std::string& c, const std::string& n, const std::string& ac, const std::string& wp);
        Route(const Route& other);
        Route& operator=(const Route& other);
        
        std::string getCallsign() const;
        std::string getCode() const;
        std::string getNumber() const;
        std::string getAirlineCode() const;
        std::vector<std::string> getWaypoints() const;
        
        // 문자열을 '-'로 분리하여 벡터에 저장하는 함수 (매개변수 방식)
        void splitStringByDash(const std::string& input, std::vector<std::string>& result) const;
};
#endif