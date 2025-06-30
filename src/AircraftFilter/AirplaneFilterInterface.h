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
    boost::geometry::model::polygon<boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>>> polygon_t;
    boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point_t;
    
    public: 
    virtual ~AirplaneFilterInterface() {}
    
    virtual void serFilterArea(const TArea& area) = 0;
    virtual void updateFilterArea(const TArea& area, const std::string filter_index) = 0;
    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) = 0;
    // Method to filter airplanes based on a specific criterion
    virtual bool filterAirplane() = 0;
    // Method to get the filtered list of airplanes
    virtual bool getFilteredAirplanes() const = 0;
    // Method to clear the filter
    virtual void ActivateFilter() = 0;
    virtual void DeactivateFilter() = 0;
    virtual bool isFilterActive() const = 0;
};

#endif