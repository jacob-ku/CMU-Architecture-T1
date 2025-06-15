#include "AbstractMapProvider.h"
#include <sys/stat.h>
#include <errno.h>
#include <vcl.h>
#include <System.SysUtils.hpp>
#include <dir.h>

AbstractMapProvider::AbstractMapProvider() : earthView(nullptr), storage(nullptr),
                                             keyhole(nullptr), tileManager(nullptr), masterLayer(nullptr)
{
}

AbstractMapProvider::~AbstractMapProvider()
{
    CleanUp();
}

void AbstractMapProvider::Resize(int width, int height)
{
    if (earthView)
        earthView->Resize(width, height);
}

FlatEarthView *AbstractMapProvider::GetEarthView() const
{
    return earthView;
}

TileManager *AbstractMapProvider::GetTileManager() const
{
    return tileManager;
}

void AbstractMapProvider::CleanUp()
{
    if (earthView)
    {
        delete earthView;
        earthView = nullptr;
    }
    if (tileManager)
    {
        delete tileManager;
        tileManager = nullptr;
    }
    if (masterLayer)
    {
        delete masterLayer;
        masterLayer = nullptr;
    }
    if (storage)
    {
        delete storage;
        storage = nullptr;
    }
    if (keyhole)
    {
        delete keyhole;
        keyhole = nullptr;
    }
}

void AbstractMapProvider::Initialize(const MapSettings &settings)
{
    CleanUp();

    AnsiString homeDir = ExtractFilePath(ExtractFileDir(Application->ExeName));
    homeDir += settings.baseDirectory.c_str();
    if (settings.loadFromInternet)
        homeDir += settings.liveDirectory.c_str();
    else
        homeDir += "\\";
    std::string cachedir = homeDir.c_str();

    if (mkdir(cachedir.c_str()) != 0 && errno != EEXIST)
    {
        throw Sysutils::Exception("Can not create cache directory");
    }

    storage = new FilesystemStorage(cachedir, true);
    if (settings.loadFromInternet)
    {
        keyhole = new KeyholeConnection(settings.mapType);
        keyhole->SetSaveStorage(storage);
        storage->SetNextLoadStorage(keyhole);
    }
    tileManager = new TileManager(storage);
    masterLayer = new GoogleLayer(tileManager);
    earthView = new FlatEarthView(masterLayer);
}