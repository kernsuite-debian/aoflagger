#ifndef RUNNER_H
#define RUNNER_H

#include "options.h"

#include "../imagesets/imageset.h"

#include <boost/optional/optional.hpp>

class Runner {
 public:
  Runner(const Options& cmdLineOptions) : _cmdLineOptions(cmdLineOptions) {}

  void Run();

 private:
  struct FileOptions {
    std::string filename;
    size_t nIntervals = 1;
    size_t intervalIndex = 0;
    size_t resolvedIntStart = 0, resolvedIntEnd = 0;
    boost::optional<size_t> intervalStart, intervalEnd;
  };
  void run(const Options& options);
  void loadStrategy(class LuaThreadGroup& lua, const Options& options,
                    const std::unique_ptr<rfiStrategy::ImageSet>& imageSet);

  void processFile(const Options& options, const std::string& filename,
                   size_t threadCount);
  std::unique_ptr<rfiStrategy::ImageSet> initializeImageSet(
      const Options& options, FileOptions& fileOptions);
  void writeHistory(const Options& options, const std::string& filename);
  void finishStatistics(const std::string& filename,
                        class ScriptData& scriptData, bool isMS);
  Options _cmdLineOptions;
};

#endif
