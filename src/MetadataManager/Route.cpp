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
    return waypoints; // Return the vector of waypoints directly
}
// std::vector<std::string> Route::getWaypoints() const {
//     std::vector<std::string> result;
    
//     if (waypoints.empty()) {
//         return result;
//     }
    
//     std::string current = "";
//     for (size_t i = 0; i < waypoints.length(); ++i) {
//         if (waypoints[i] == '-') {
//             if (!current.empty()) {
//                 result.push_back(current);
//                 current = "";
//             }
//         } else {
//             current += waypoints[i];
//         }
//     }
    
//     // Add the last waypoint if not empty
//     if (!current.empty()) {
//         result.push_back(current);
//     }
    
//     return result;
// }

// 문자열을 '-'로 분리하여 벡터에 저장하는 함수 (매개변수 방식)
void Route::splitStringByDash(const std::string& input, std::vector<std::string>& result) const {
    // 기존 결과 벡터를 클리어
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
    
    // Add the last part if not empty
    if (!current.empty()) {
        result.push_back(current);
    }
}
