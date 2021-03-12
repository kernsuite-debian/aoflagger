#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"

#include "../../algorithms/sumthresholdmissing.h"
#include "../../algorithms/testsetgenerator.h"
#include "../../algorithms/thresholdconfig.h"

#include "../testingtools/maskasserter.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(masked_sumthreshold,
                      *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(horizontal) {
  const unsigned width = 8, height = 8;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height),
         missing = Mask2D::MakeSetMask<false>(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  Image2D image = Image2D::MakeSetImage(width, height, 0.0);

  for (size_t y = 0; y != height; ++y) {
    image.SetValue(3, y, 1.0);
    image.SetValue(4, y, 1.0);
  }

  SumThresholdMissing::Horizontal(image, mask, missing, scratch, 2, 0.8);

  for (size_t y = 0; y != height; ++y) {
    BOOST_CHECK(mask.Value(3, y));
    BOOST_CHECK(mask.Value(4, y));
    BOOST_CHECK(!mask.Value(0, y));
    BOOST_CHECK(!mask.Value(2, y));
    BOOST_CHECK(!mask.Value(5, y));
  }
}

BOOST_AUTO_TEST_SUITE_END()
