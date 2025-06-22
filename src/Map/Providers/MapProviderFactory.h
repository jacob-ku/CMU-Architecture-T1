#pragma once
#include "MapProvider.h"

// Example map type enum values
#define GoogleMaps 0
#define SkyVector_VFR 1
#define SkyVector_IFR_Low 2
#define SkyVector_IFR_High 3
// Add other map type defines as needed

class MapProviderFactory
{
public:
  static MapProvider *Create(int mapType);
};
