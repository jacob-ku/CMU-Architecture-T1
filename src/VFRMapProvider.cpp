#include "VFRMapProvider.h"

void VFRMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\VFR_Map\\";
    settings.mapType = SkyVector_VFR;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
}
