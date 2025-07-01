#include "Airport.h"

Airport::Airport(const std::string& c, const std::string& n, const std::string& ic,
    const std::string& ia, const std::string& co, const float lat, const float lon,
    const int alt)
    : code(c), name(n), icao(ic), iata(ia), country(co), latitude(lat), longitude(lon), altitude(alt) {}

Airport::Airport(): code(""), name(""), icao(""), iata(""), country(""), latitude(0.0f), longitude(0.0f), altitude(0) {}

Airport::~Airport() {
}

const std::string& Airport::getCode() const {
    return code;
}

const std::string& Airport::getName() const {
    return name;
}

const std::string& Airport::getICAO() const {
    return icao;
}

const std::string& Airport::getIATA() const {
    return iata;
}

const std::string& Airport::getCountry() const {
    return country;
}

float Airport::getLatitude() const {
    return latitude;
}

float Airport::getLongitude() const {
    return longitude;
}

int Airport::getAltitude() const {
    return altitude;
}
