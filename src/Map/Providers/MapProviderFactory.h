#pragma once
#include "MapProvider.h"

// Google Maps tile type enum values
#define GoogleMaps 0
#define GoogleMaps_Street 1         // lyrs=m
#define GoogleMaps_TerrainLabels 2  // lyrs=p

// Other map type enum values
#define SkyVector_VFR 3
#define SkyVector_IFR_Low 4
#define SkyVector_IFR_High 5
// Add other map type defines as needed

class MapProviderFactory
{
public:
  static MapProvider *Create(int mapType);
};
