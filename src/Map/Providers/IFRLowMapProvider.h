#pragma once
#include "AbstractMapProvider.h"

class IFRLowMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
};
