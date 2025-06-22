#pragma once
#include "AbstractMapProvider.h"

class IFRHighMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
};
