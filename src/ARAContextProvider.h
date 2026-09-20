#pragma once

#include "HarmonicContext.h"

namespace smartvoicing::harmony
{
class ARAContextProvider final : public IHarmonicContextProvider
{
public:
    HarmonicContext currentContext() noexcept override;
    HarmonicContext contextAt(double ppq) noexcept override;
};
}
