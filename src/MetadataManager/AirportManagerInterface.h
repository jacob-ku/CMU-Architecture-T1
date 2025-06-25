#include "AirportDB.h"
#include <unordered_map>
#include <string>

interface AirportManagerInterface
{
    public:
    virtual ~AirportManagerInterface() = default;

    // Method to get the name of an airport by its code
    //virtual bool loadAirport(const std::string& airportCode, std::string& airportName) const = 0;
    virtual bool LoadAirport(const std::string& sourcefile = "") = 0;
    virtual std::unordered_map<std::string, Airport>& getAirportCodeMap() = 0; 
    virtual Airport& getAirportByCode(const std::string& code) = 0;

    // Method to list all airports
    virtual std::vector<std::string> getAirportList() const = 0;
    
};
