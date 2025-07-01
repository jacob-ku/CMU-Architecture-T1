#include "Route.h"

Route::Route(const std::string& cs, const std::string& c, const std::string& n, const std::string& ac, const std::string& wp)
    : callsign(cs), code(c), number(n), airlinecoode(ac), waypointsStr(wp) {

    splitStringByDash(wp, waypoints);
}

Route::Route(const Route& other)
    : callsign(other.callsign), code(other.code), number(other.number), airlinecoode(other.airlinecoode), waypoints(other.waypoints), waypointsStr(other.waypointsStr) {
}

Route& Route::operator=(const Route& other) {
    if (this != &other) {
        callsign = other.callsign;
        code = other.code;
        number = other.number;
        airlinecoode = other.airlinecoode;
        waypoints = other.waypoints;
        waypointsStr = other.waypointsStr;
    }
    return *this;
}

const std::string& Route::getCallsign() const {
    return callsign;
}

const std::string& Route::getCode() const {
    return code;
}

const std::string& Route::getNumber() const {
    return number;
}

const std::string& Route::getAirlineCode() const {
    return airlinecoode;
}

const std::vector<std::string>& Route::getWaypoints() const {
    return waypoints; 
}

const std::string& Route::getWaypointStr() const {
    return waypointsStr;
}

void Route::splitStringByDash(const std::string& input, std::vector<std::string>& result) const {
    result.clear();
    
    if (input.empty()) {
        return;
    }

    // split input by '-' and push to the result vector
    size_t start = 0;
    size_t end = input.find('-');
    while (end != std::string::npos) {
        std::string token = input.substr(start, end - start);
        if (!token.empty()) {
            result.push_back(token);
        }
        start = end + 1;
        end = input.find('-', start);
    }

    std::string token = input.substr(start, end - start);
    if (!token.empty()) {
        result.push_back(token);
    }
}
