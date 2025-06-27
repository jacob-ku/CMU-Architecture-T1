#include "MapProviderFactory.h"
#include "GoogleMapProvider.h"
#include "GoogleStreetViewMapProvider.h"
#include "GoogleTerrainWithLabelsMapProvider.h"
#include "VFRMapProvider.h"
#include "IFRLowMapProvider.h"
#include "IFRHighMapProvider.h"

MapProvider *MapProviderFactory::Create(int mapType)
{
    switch (mapType)
    {
    case GoogleMaps:
        return new GoogleMapProvider();
    case GoogleMaps_Street:
        return new GoogleStreetViewMapProvider();
    case GoogleMaps_TerrainLabels:
        return new GoogleTerrainWithLabelsMapProvider();
    case SkyVector_VFR:
        return new VFRMapProvider();
    case SkyVector_IFR_Low:
        return new IFRLowMapProvider();
    case SkyVector_IFR_High:
        return new IFRHighMapProvider();
    // Add other cases for different providers
    default:
        return nullptr;
    }
}
