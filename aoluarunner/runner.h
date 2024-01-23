#ifndef RUNNER_H
#define RUNNER_H

#include "options.h"

#include "../imagesets/imageset.h"

#include <optional>
#include <memory>
#include <string>
#include <vector>

struct ChunkInfo;

class Runner {
 public:
  explicit Runner(const Options& cmdLineOptions)
      : _cmdLineOptions(cmdLineOptions) {}

  void Run();

 private:
  struct FileOptions {
    std::string filename;
    size_t nIntervals = 1;
    size_t intervalIndex = 0;
    size_t resolvedIntStart = 0, resolvedIntEnd = 0;
    std::optional<size_t> intervalStart, intervalEnd;
  };
  void run(const Options& options);
  void loadStrategy(class LuaThreadGroup& lua, const Options& options,
                    const std::unique_ptr<imagesets::ImageSet>& imageSet);

  void processFile(const Options& options, const std::string& filename,
                   size_t threadCount);
  void processFrequencyConcatenatedFiles(
      Options options, const std::vector<std::string>& filenames,
      size_t n_threads);
  void ProcessFrequencyConcatenatedFilesChunk(
      const Options& options, const std::vector<std::string>& ms_names,
      size_t n_threads, size_t n_io_threads,
      const std::optional<ChunkInfo>& chunk_info);
  std::unique_ptr<imagesets::ImageSet> initializeImageSet(
      const Options& options, FileOptions& fileOptions);
  void writeHistory(const Options& options, const std::string& filename);
  void finishStatistics(const std::string& filename,
                        class ScriptData& scriptData, bool isMS);
  Options _cmdLineOptions;
};

#endif
