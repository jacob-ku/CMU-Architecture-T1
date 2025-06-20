#pragma once
#include "../MapSrc/FilesystemStorage.h"
#include "../MapSrc/FlatEarthView.h"
#include "../MapSrc/GoogleLayer.h"
#include "../MapSrc/KeyholeConnection.h"
#include "MapProvider.h"
#include "../MapSrc/TileManager.h"

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
