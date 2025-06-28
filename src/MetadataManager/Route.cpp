#include "Route.h"

Route::Route(const std::string& cs, const std::string& c, const std::string& n, const std::string& ac, const std::string& wp)
    : callsign(cs), code(c), number(n), airlinecoode(ac), waypointsStr(wp) {

    splitStringByDash(wp, waypoints);
}

Route::Route(const Route& other)
    : callsign(other.callsign), code(other.code), number(other.number), airlinecoode(other.airlinecoode), waypoints(other.waypoints) {
}

Route& Route::operator=(const Route& other) {
    if (this != &other) {
        callsign = other.callsign;
        code = other.code;
        number = other.number;
        airlinecoode = other.airlinecoode;
        waypoints = other.waypoints;
    }
    return *this;
}

std::string Route::getCallsign() const {
    return callsign;
}

std::string Route::getCode() const {
    return code;
}

std::string Route::getNumber() const {
    return number;
}

std::string Route::getAirlineCode() const {
    return airlinecoode;
}

std::vector<std::string> Route::getWaypoints() const {
    return waypoints; 
}

void Route::splitStringByDash(const std::string& input, std::vector<std::string>& result) const {
    result.clear();
    
    if (input.empty()) {
        return;
    }
    
    std::string current = "";
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '-') {
            if (!current.empty()) {
                result.push_back(current);
                current = "";
            }
        } else {
            current += input[i];
        }
    }

    if (!current.empty()) {
        result.push_back(current);
    }
}
