#include "OpenStreetMapProvider.h"

void OpenStreetMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\MapData\\Open_Street_Map\\";
    settings.mapType = Open_Street;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
}
