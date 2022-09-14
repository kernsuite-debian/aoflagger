#include <boost/test/unit_test.hpp>

#include "../../algorithms/testsetgenerator.h"
#include "../../structures/timefrequencydata.h"
#include "../../interface/aoflagger.h"
#include "../../util/stopwatch.h"
#include "../../lua/telescopefile.h"

#include <aocommon/system.h>

#include <iostream>
#include <thread>
#include <vector>

using aoflagger::AOFlagger;
using aoflagger::ImageSet;
using aoflagger::Strategy;

using algorithms::BackgroundTestSet;
using algorithms::RFITestSet;
using algorithms::TestSetGenerator;

BOOST_AUTO_TEST_SUITE(throughput, *boost::unit_test::label("experiment") *
                                      boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(default_pipeline) {
  const std::vector<TelescopeFile::TelescopeId> telescopeList =
      TelescopeFile::List();
  for (TelescopeFile::TelescopeId telescope : telescopeList) {
    const size_t width = 4096;
    const size_t height = 256;
    const size_t nRepeat = 10;
    TimeFrequencyData data = TestSetGenerator::MakeTestSet(
        RFITestSet::GaussianBursts, BackgroundTestSet::Empty, width, height);

    AOFlagger aoflagger;
    const std::string filename =
        aoflagger.FindStrategyFile(aoflagger::TelescopeId(telescope));

    Stopwatch watch(true);
    std::vector<std::thread> threads;
    const size_t nThreads = aocommon::system::ProcessorCount();
    for (size_t threadIndex = 0; threadIndex != nThreads; ++threadIndex) {
      threads.emplace_back([&, threadIndex]() {
        Strategy strategy = aoflagger.LoadStrategyFile(filename);
        ImageSet imageSet =
            aoflagger.MakeImageSet(width, height, data.ImageCount());
        for (size_t i = 0; i != data.ImageCount(); ++i) {
          float* dest = imageSet.ImageBuffer(i);
          Image2DCPtr input = data.GetImage(i);
          for (size_t y = 0; y != height; ++y) {
            std::copy_n(input->ValuePtr(0, y), width,
                        dest + y * imageSet.HorizontalStride());
          }
        }
        try {
          for (size_t i = 0; i != nRepeat; ++i) strategy.Run(imageSet);
        } catch (std::exception& e) {
          if (threadIndex == 0) std::cout << "ERROR: " << e.what() << std::endl;
        }
      });
    }
    for (std::thread& t : threads) t.join();

    const uint64_t imageSetVolume =
        width * height * data.ImageCount() * sizeof(float);
    const uint64_t totalVolume = imageSetVolume * nThreads * nRepeat;
    long double totalTime = watch.Seconds();
    std::cout << "Telescope: " << TelescopeFile::TelescopeName(telescope)
              << '\n'
              << "nTimes = " << width << '\n'
              << "nChannels = " << height << '\n'
              << "nImages = " << data.ImageCount() << '\n'
              << "Total volume = " << totalVolume / (1024 * 1024) << " MB\n"
              << "Total time = " << watch.ToString() << '\n'
              << "Throughput = " << totalVolume / (1024 * 1024) / totalTime
              << " MB/s\n\n";
  }
}

BOOST_AUTO_TEST_SUITE_END()
