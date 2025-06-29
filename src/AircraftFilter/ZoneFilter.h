#ifndef ZONEFILTER_H
#define ZONEFILTER_H

#include "AirplaneFilterInterface.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <vector>
#include <string>

// namespace boost::geometry = boost::geometry;
// typedef boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point_t;
// typedef boost::geometry::model::polygon<point_t> polygon_t;

class ZoneFilter : public AirplaneFilterInterface {
private:
    bool filterActive;
    boost::geometry::model::polygon<boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>>> filterPolygon;
    std::vector<TADS_B_Aircraft> filteredAirplanes;
    std::string currentFilterIndex;

public:    
    ZoneFilter();
    
    virtual ~ZoneFilter();
    
    virtual void serFilterArea(const TArea& area) override;
    virtual void updateFilterArea(const TArea& area, const std::string filter_index) override;
    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) override;
    virtual bool filterAirplane() override;
    virtual bool getFilteredAirplanes() const override;
    virtual void ActivateFilter() override;
    virtual void DeactivateFilter() override;
    
    void setPolygonFromArea(const TArea& area);
    bool isPointInPolygon(double latitude, double longitude) const;
    void clearFilteredAirplanes();
    size_t getFilteredAirplanesCount() const;
    bool isFilterActive() const;    
    void printPolygonInfo() const;
};

#endif
