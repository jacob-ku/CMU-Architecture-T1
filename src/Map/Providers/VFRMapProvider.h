#pragma once
#include "AbstractMapProvider.h"

class VFRMapProvider : public AbstractMapProvider
{
public:
  void Initialize(bool loadFromInternet) override;
};
