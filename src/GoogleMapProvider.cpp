#include "GoogleMapProvider.h"

void GoogleMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\GoogleMap\\";
    settings.liveDirectory = "_Live\\";
    settings.mapType = GoogleMaps;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
}
