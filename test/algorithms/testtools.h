#ifndef TEST_ALGORITHMS_TESTTOOLS_H
#define TEST_ALGORITHMS_TESTTOOLS_H

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

#include <functional>
#include <set>

namespace test_tools {

using SumThresholdFunction =
    std::function<void(const Image2D*, Mask2D*, Mask2D*, size_t, num_t)>;

void CompareVerticalSumThreshold(SumThresholdFunction algorithm,
                                 const std::set<size_t>& skips);

void CompareHorizontalSumThreshold(SumThresholdFunction algorithm);

void IntroduceGap(const Image2D& input, const Mask2D& mask, Image2D& mInput,
                  Mask2D& mMask, Mask2D& missing);

void RemoveGap(Mask2D& mask, const Mask2D& mMask);

}  // namespace test_tools

#endif
