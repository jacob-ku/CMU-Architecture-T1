#pragma once
#include "AbstractMapProvider.h"

class GoogleStreetViewMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
}; 