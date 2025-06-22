#include "GoogleMapProvider.h"

void GoogleMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\GoogleMap\\";
    settings.mapType = GoogleMaps;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
}
