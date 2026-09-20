#pragma once

#include "HarmonicContext.h"

namespace smartvoicing::harmony
{
class ARAContextProvider final : public IHarmonicContextProvider
{
public:
    HarmonicContext currentContext() noexcept override;
    HarmonicContext contextAt(double ppq) noexcept override;
    double nextChordStartAfter(double ppq) noexcept override;
    double secondsAtPpq(double ppq) noexcept override;
    double ppqAtSeconds(double seconds) noexcept override;
};
}
