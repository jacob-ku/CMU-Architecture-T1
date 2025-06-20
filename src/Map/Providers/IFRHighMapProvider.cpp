#include "IFRHighMapProvider.h"

void IFRHighMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\IFR_High_Map\\";
    settings.mapType = SkyVector_IFR_High;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
}
