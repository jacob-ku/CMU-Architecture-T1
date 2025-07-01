#ifndef AIRPORT_H
#define AIRPORT_H

#include <string>

class Airport
{
private:
    std::string code;
    std::string name;
    std::string icao;
    std::string iata;
    std::string country;
    float latitude;
    float longitude;
    int altitude;

public:
    Airport(const std::string& c, const std::string& n, const std::string& ic,
        const std::string& ia, const std::string& co, const float lat,
        const float lon, const int alt);
    Airport();
    ~Airport();
   
    const std::string& getCode() const;
    const std::string& getName() const;
    const std::string& getICAO() const;
    const std::string& getIATA() const;
    const std::string& getCountry() const;
    float getLatitude() const;
    float getLongitude() const;
    int getAltitude() const; 
};
#endif // AIRPORT_H