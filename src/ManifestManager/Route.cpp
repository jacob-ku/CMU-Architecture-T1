#include "Route.h"

Route::Route(const std::string& cs, const std::string& c, const std::string& n, const std::string& ac, const std::vector<std::string>& wp)
    : callsign(cs), code(c), number(n), airlinecoode(ac), waypoints(wp) {
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
