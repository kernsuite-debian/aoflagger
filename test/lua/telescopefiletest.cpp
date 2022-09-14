#include "../../lua/telescopefile.h"
#include "../../interface/aoflagger.h"

#include <filesystem>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(strategy_files, *boost::unit_test::label("lua"))

BOOST_AUTO_TEST_CASE(find_strategy) {
  std::filesystem::path argv0 = std::filesystem::current_path() / "aoflagger";

  auto all = TelescopeFile::List();
  for (TelescopeFile::TelescopeId telescope : all) {
    std::string path =
        TelescopeFile::FindStrategy(argv0.string(), telescope, "");
    BOOST_CHECK_NE(path, "");
  }
}

BOOST_AUTO_TEST_CASE(find_non_existing_strategy) {
  std::filesystem::path argv0 = std::filesystem::current_path() / "aoflagger";

  std::string nonExisting = TelescopeFile::FindStrategy(
      argv0.string(), TelescopeFile::GENERIC_TELESCOPE, "_NonExisting");
  BOOST_CHECK_EQUAL(nonExisting, "");
}

void runStrategy(TelescopeFile::TelescopeId telescope) {
  std::filesystem::path argv0 = std::filesystem::current_path() / "aoflagger";

  aoflagger::AOFlagger flagger;
  const size_t pols = 8, width = 7, height = 10;
  aoflagger::ImageSet imageSet = flagger.MakeImageSet(width, height, pols);
  for (size_t p = 0; p != pols; ++p) {
    for (size_t y = 0; y != height; ++y) {
      float* row = imageSet.ImageBuffer(p) + y * imageSet.HorizontalStride();
      for (size_t x = 0; x != width; ++x) {
        row[x] = ((x + y) % 2 == 0) ? -1 : 2;
      }
    }
  }

  aoflagger::Band band;
  band.id = 0;
  for (size_t i = 0; i != height; ++i)
    band.channels.emplace_back(
        aoflagger::Channel{(1380 + i * 10) * 1e6, 1 * 1e6});
  flagger.SetBandList(std::vector<aoflagger::Band>{band});
  imageSet.SetBand(0);

  // We set one absurdly high value which all strategies should detect
  size_t rfiX = width / 2, rfiY = height / 2, rfiP = 5;
  float* rfiPixel =
      imageSet.ImageBuffer(rfiP) + rfiY * imageSet.HorizontalStride() + rfiX;
  *rfiPixel = 1000.0;

  std::string path = TelescopeFile::FindStrategy(argv0.string(), telescope, "");
  BOOST_CHECK(!path.empty());
  aoflagger::Strategy strategy = flagger.LoadStrategyFile(path);
  aoflagger::FlagMask mask = strategy.Run(imageSet);
  bool* rfiPixelFlag = mask.Buffer() + rfiY * mask.HorizontalStride() + rfiX;
  BOOST_CHECK(*rfiPixelFlag);
}

// This is pretty repetative, but this way it is immeditely clear which strategy
// fails, if one fails.
BOOST_AUTO_TEST_CASE(run_generic) {
  runStrategy(TelescopeFile::GENERIC_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_aartfaac) {
  runStrategy(TelescopeFile::AARTFAAC_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_apertif) {
  runStrategy(TelescopeFile::APERTIF_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_arecibo) {
  runStrategy(TelescopeFile::ARECIBO_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_atca) { runStrategy(TelescopeFile::ATCA_TELESCOPE); }

BOOST_AUTO_TEST_CASE(run_bighorns) {
  runStrategy(TelescopeFile::BIGHORNS_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_jvla) { runStrategy(TelescopeFile::JVLA_TELESCOPE); }

BOOST_AUTO_TEST_CASE(run_lofar) { runStrategy(TelescopeFile::LOFAR_TELESCOPE); }

BOOST_AUTO_TEST_CASE(run_mwa) { runStrategy(TelescopeFile::MWA_TELESCOPE); }

BOOST_AUTO_TEST_CASE(run_nenufar) {
  runStrategy(TelescopeFile::NENUFAR_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_parkes) {
  runStrategy(TelescopeFile::PARKES_TELESCOPE);
}

BOOST_AUTO_TEST_CASE(run_wsrt) { runStrategy(TelescopeFile::WSRT_TELESCOPE); }

BOOST_AUTO_TEST_SUITE_END()
