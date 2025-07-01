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
protected:
    double maxLatitude;
    double minLatitude;
    double maxLongitude;
    double minLongitude;

    double maxinnerLatitude;
    double mininnerLatitude;
    double maxinnerLongitude;
    double mininnerLongitude;

public:    
    ZoneFilter();
    
    virtual ~ZoneFilter();
    
    virtual void serFilterArea(const TArea& area);
    virtual void updateFilterArea(const TArea& area, const std::string filter_index);
    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) override;
    virtual bool ClearFilter() override;
    virtual bool getFilteredAirplanes() const override;
    virtual void ActivateFilter() override;
    virtual void DeactivateFilter() override;
    virtual void setAndFilter(bool value) override;
    
    void setPolygonFromArea(const TArea& area);
    bool isPointInPolygon(double latitude, double longitude) const;
    void clearFilteredAirplanes();
    size_t getFilteredAirplanesCount() const;
    virtual bool isFilterActive() const override;    
    void printPolygonInfo() const;
    
    // Get visual center of the polygon using envelope center method
    double getMaxLatitude() const;
    double getMinLatitude() const;
    double getMaxLongitude() const;
    double getMinLongitude() const;
    void getPolygonBounds(double& maxLat, double& minLat, double& maxLon, double& minLon) const;
    
    bool isPointInOutBox(double latitude, double longitude) const;
    bool isPointInInBox(double latitude, double longitude) const; 
    bool getVisualCenter(double& longitude, double& latitude) const;
    
    // Get maximum axis-aligned rectangle inscribed in polygon from center
    // Returns rectangle bounds: minLon, maxLon, minLat, maxLat
    bool getMaxInscribedRectangle(const boost::geometry::model::polygon<boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>>>& polygon,
                                 double& minLon, double& maxLon, double& minLat, double& maxLat) const;
};

#endif
