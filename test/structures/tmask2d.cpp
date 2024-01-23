#include "../../structures/mask2d.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(mask2d, *boost::unit_test::label("structures"))

BOOST_AUTO_TEST_CASE(create_ptr_from_rows_zero_size_uninitialized_source) {
  const Mask2D source = Mask2D::MakeUnsetMask(0, 0);
  const Mask2DPtr mask = Mask2D::CreatePtrFromRows(source, 0, 0);
  BOOST_CHECK_EQUAL(mask->Width(), 0);
  BOOST_CHECK_EQUAL(mask->Height(), 0);
  BOOST_CHECK_NE(mask->Data(), nullptr);
}

BOOST_AUTO_TEST_CASE(create_ptr_from_uninitialized_rows_zero_size_destination) {
  const Mask2D source = Mask2D::MakeUnsetMask(42, 42);
  const Mask2DPtr mask = Mask2D::CreatePtrFromRows(source, 0, 0);
  BOOST_CHECK_EQUAL(mask->Width(), 42);
  BOOST_CHECK_EQUAL(mask->Height(), 0);
  BOOST_CHECK_NE(mask->Data(), nullptr);
}

BOOST_AUTO_TEST_CASE(create_ptr_from_uninitialized_rows_zero) {
  Mask2D source = Mask2D::MakeUnsetMask(42, 2);
  for (size_t x = 0; x < 42; ++x) source.SetValue(x, 0, x % 2);

  const Mask2DPtr mask = Mask2D::CreatePtrFromRows(source, 0, 1);
  BOOST_CHECK_EQUAL(mask->Width(), 42);
  BOOST_CHECK_EQUAL(mask->Height(), 1);
  BOOST_CHECK_NE(mask->Data(), nullptr);
  for (size_t x = 0; x < 42; ++x) BOOST_CHECK_EQUAL(mask->Value(x, 0), x % 2);
}

BOOST_AUTO_TEST_CASE(create_ptr_from_initialized_rows_zero) {
  Mask2D source = Mask2D::MakeSetMask<false>(42, 2);

  const Mask2DPtr mask = Mask2D::CreatePtrFromRows(source, 1, 1);
  BOOST_CHECK_EQUAL(mask->Width(), 42);
  BOOST_CHECK_EQUAL(mask->Height(), 1);
  BOOST_CHECK_NE(mask->Data(), nullptr);
  for (size_t x = 0; x < 42; ++x) BOOST_CHECK(mask->Value(x, 0) == false);
}

BOOST_AUTO_TEST_CASE(to_string) {
  Mask2D a = Mask2D::MakeSetMask<false>(2, 3);
  BOOST_CHECK_EQUAL(a.ToString(),
                    "  \n"
                    "  \n"
                    "  \n");

  Mask2D b = Mask2D::MakeSetMask<true>(2, 3);
  BOOST_CHECK_EQUAL(b.ToString(),
                    "XX\n"
                    "XX\n"
                    "XX\n");
  Mask2D c = Mask2D::MakeSetMask<false>(2, 3);
  c.SetValue(0, 0, true);
  c.SetValue(1, 1, true);
  c.SetValue(1, 2, true);
  BOOST_CHECK_EQUAL(c.ToString(),
                    "X \n"
                    " X\n"
                    " X\n");
}

BOOST_AUTO_TEST_SUITE_END()
