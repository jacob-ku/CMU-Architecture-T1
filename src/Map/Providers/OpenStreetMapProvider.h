#pragma once
#include "AbstractMapProvider.h"

class OpenStreetMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
};
