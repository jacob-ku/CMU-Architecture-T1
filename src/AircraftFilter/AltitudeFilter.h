#ifndef ALTITUDEFILTER_H
#define ALTITUDEFILTER_H
#include "AirplaneFilterInterface.h"
#include <string>

class AltitudeFilter : public AirplaneFilterInterface {
private:
    double minAltitude;
    double maxAltitude;
    bool filterActive;
public:
    AltitudeFilter(double minAlt = 0.0, double maxAlt = 100000.0);
    virtual ~AltitudeFilter();
    
    // AirplaneFilterInterface required functions
    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) override;
    virtual bool ClearFilter() override;
    virtual bool getFilteredAirplanes() const override;
    virtual void ActivateFilter() override;
    virtual void DeactivateFilter() override;
    virtual bool isFilterActive() const override;
    virtual void setAndFilter(bool value) override;
    
    // AltitudeFilter specific functions
    virtual void setAltitudeRange(double minAlt, double maxAlt);
};

#endif