#include "GoogleTerrainWithLabelsMapProvider.h"

void GoogleTerrainWithLabelsMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\MapData\\Google_TerrainLabels_Map\\";
    settings.mapType = GoogleMaps_TerrainLabels;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
} 