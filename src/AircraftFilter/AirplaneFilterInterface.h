#ifndef AIRPLANEFILTERINTERFACE_H
#define AIRPLANEFILTERINTERFACE_H   

#include <string>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include "Aircraft.h"
#include "TArea.h"

interface AirplaneFilterInterface {
    private:
    bool filterActive;
    bool isAndFilter;

    public: 
    AirplaneFilterInterface() : isAndFilter(false) {}
    virtual ~AirplaneFilterInterface() {}
    
    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) = 0;
    // Method to filter airplanes based on a specific criterion
    virtual bool ClearFilter() = 0;
    // Method to get the filtered list of airplanes
    virtual bool getFilteredAirplanes() const = 0;
    // Method to clear the filter
    virtual void ActivateFilter() = 0;
    virtual void DeactivateFilter() = 0;
    virtual bool isFilterActive() const = 0;
    virtual void setAndFilter(bool value) = 0;
};

#endif