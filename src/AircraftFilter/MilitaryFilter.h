#ifndef MILITARYFILTER_H
#define MILITARYFILTER_H

#include "AirplaneFilterInterface.h"
#include <iostream>

class MilitaryFilter : public AirplaneFilterInterface {
private:
    bool filterActive;
    bool isAndFilter;

public:
    MilitaryFilter();
    virtual ~MilitaryFilter();
    
    // Pure virtual functions from AirplaneFilterInterface
    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) override;
    virtual bool ClearFilter() override;
    virtual bool getFilteredAirplanes() const override;
    virtual void ActivateFilter() override;
    virtual void DeactivateFilter() override;
    virtual bool isFilterActive() const override;
    virtual void setAndFilter(bool value) override;
    virtual bool getAndfilter() const override;
};

#endif