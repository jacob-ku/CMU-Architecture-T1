#include "Airport.h"

Airport::Airport(std::string c, std::string n, std::string ic, std::string ia, std::string co, float lat, float lon, int alt)
    : code(c), name(n), icao(ic), iata(ia), country(co), latitude(lat), longitude(lon), altitude(alt) {
}
Airport::Airport(): code(""), name(""), icao(""), iata(""), country(""), latitude(0.0f), longitude(0.0f), altitude(0) {
}

Airport::~Airport() {
}

std::string Airport::getCode() const {
    return code;
}

std::string Airport::getName() const {
    return name;
}

std::string Airport::getICAO() const {
    return icao;
}

std::string Airport::getIATA() const {
    return iata;
}

std::string Airport::getCountry() const {
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
