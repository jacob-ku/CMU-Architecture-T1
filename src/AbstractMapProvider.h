#pragma once
#include "FilesystemStorage.h"
#include "FlatEarthView.h"
#include "GoogleLayer.h"
#include "KeyholeConnection.h"
#include "MapProvider.h"
#include "TileManager.h"

class AbstractMapProvider : public MapProvider
{
public:
  AbstractMapProvider();
  ~AbstractMapProvider();
  void Resize(int width, int height) override;
  FlatEarthView *GetEarthView() const override;
  TileManager *GetTileManager() const override;
  virtual void Initialize(bool loadFromInternet) override = 0;

protected:
  struct MapSettings
  {
    std::string baseDirectory;
    int mapType;
    bool loadFromInternet;
  };
  FlatEarthView *earthView;
  FilesystemStorage *storage;
  KeyholeConnection *keyhole;
  TileManager *tileManager;
  GoogleLayer *masterLayer;
  void CleanUp();
  void Initialize(const MapSettings &settings);
};
