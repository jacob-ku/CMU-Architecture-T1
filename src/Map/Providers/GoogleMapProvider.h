#pragma once
#include "AbstractMapProvider.h"

class GoogleMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
};
