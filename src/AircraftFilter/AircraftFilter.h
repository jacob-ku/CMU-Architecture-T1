#ifndef AIRPLANEFILTER_H
#define AIRPLANEFILTER_H

#include "AirplaneFilterInterface.h"
#include "ZoneFilter.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>

class AircraftFilter {
private:
    std::unordered_map<std::string, std::unique_ptr<AirplaneFilterInterface>> filters;

public:
    AircraftFilter();
    ~AircraftFilter();
    
    void addFilter(const std::string& filterName, std::unique_ptr<AirplaneFilterInterface> filterInstance);
    void removeFilter(const std::string& filterName);
    void replaceFilter(const std::string& filterName, std::unique_ptr<AirplaneFilterInterface> filterInstance);
    void activateFilter(const std::string& filterName);
    void deactivateFilter(const std::string& filterName);
    bool filterAircraft(const TADS_B_Aircraft& aircraft);
    bool filterAircraftPosition(double latitude, double longitude);

    
    // 유틸리티 메서드들
    bool hasFilter(const std::string& filterName) const;
    void clearAllFilters();
    size_t getFilterCount() const;
    void printFilterStatus() const;
};

#endif
