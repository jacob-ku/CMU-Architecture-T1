#include "GoogleStreetViewMapProvider.h"

void GoogleStreetViewMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\MapData\\Google_Street_Map\\";
    settings.mapType = GoogleMaps_Street;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
} 