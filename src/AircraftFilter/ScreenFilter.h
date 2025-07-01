#ifndef SCREENFILTER_H
#define SCREENFILTER_H

#include "ZoneFilter.h"
#include <string>

class ScreenFilter : public ZoneFilter {
public:
    ScreenFilter();
    virtual ~ScreenFilter();
    virtual void serFilterArea(const TArea& area) override;
    // Override functions from ZoneFilter/AirplaneFilterInterface if needed

    virtual bool isAirplaneIncluded(const TADS_B_Aircraft& targetair) override;
private:
    double screenMinX, screenMinY, screenMaxX, screenMaxY;
    bool screenBoundsSet;
};

#endif