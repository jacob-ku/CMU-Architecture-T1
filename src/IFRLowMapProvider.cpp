#include "IFRLowMapProvider.h"

void IFRLowMapProvider::Initialize(bool loadFromInternet)
{
    MapSettings settings;
    settings.baseDirectory = "..\\IFR_Low_Map\\";
    settings.mapType = SkyVector_IFR_Low;
    settings.loadFromInternet = loadFromInternet;

    AbstractMapProvider::Initialize(settings);
}
