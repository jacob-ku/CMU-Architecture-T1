#pragma once

#include "FlatEarthView.h"
#include "TileManager.h"

class MapProvider
{
public:
  virtual ~MapProvider() {}
  virtual void Initialize(bool loadFromInternet) = 0;
  virtual void Resize(int width, int height) = 0;
  virtual FlatEarthView *GetEarthView() const = 0;
  virtual TileManager *GetTileManager() const = 0;
};
