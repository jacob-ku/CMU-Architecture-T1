#include "AirportDB.h"
#include <unordered_map>
#include <string>

interface AirportManagerInterface
{
    public:
    virtual ~AirportManagerInterface() = default;

    virtual bool LoadAirport(const std::string& sourcefile = "") = 0;
    virtual std::unordered_map<std::string, Airport>& getAirportCodeMap() = 0; 
    virtual Airport getAirportByCode(const std::string& code) = 0;

    virtual std::vector<std::string> getAirportList() const = 0;
    
};
