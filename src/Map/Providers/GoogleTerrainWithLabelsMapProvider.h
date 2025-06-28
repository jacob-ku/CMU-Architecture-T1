#pragma once
#include "AbstractMapProvider.h"

class GoogleTerrainWithLabelsMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
}; 