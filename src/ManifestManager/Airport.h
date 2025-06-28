#include <iostream>
#ifndef AIRPORT_H_
#define AIRPORT_H_
using namespace std;

class Airport
{
    std::string code;
    std::string name;
    std::string icao;
    std::string iata;
    std::string country;
    float latitude;
    float longitude;
    int altitude;

    public:
    Airport(std::string c, std::string n, std::string ic, std::string ia, std::string co, float lat, float lon, int alt);
    Airport();

    ~Airport();    
    std::string getCode() const;
    std::string getName() const;
    std::string getICAO() const;
    std::string getIATA() const;
    std::string getCountry() const;
    float getLatitude() const;
    float getLongitude() const;
    int getAltitude() const;
   
};


#endif // AIRPORT_H_