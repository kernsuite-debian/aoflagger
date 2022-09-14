#include "runner.h"
#include "baselineiterator.h"

#include "../lua/luathreadgroup.h"
#include "../lua/scriptdata.h"
#include "../lua/optionsfunction.h"
#include "../lua/telescopefile.h"

#include "../quality/statisticscollection.h"

#include "../structures/msmetadata.h"

#include "../imagesets/joinedspwset.h"
#include "../imagesets/msimageset.h"
#include "../imagesets/msoptions.h"
#include "../imagesets/multibandmsimageset.h"

#include "../util/logger.h"

#include <aocommon/system.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <algorithm>
#include <fstream>

using namespace imagesets;

// The order of initialization:
// 1. If Lua strategy given, load Lua strategy
// 2. Determine options
// 3. For each run and each file:
//   a) If no Lua strategy was given, open file and load corresponding Lua
//   strategy b) Run strategy

void Runner::Run() {
  std::map<std::string, Options> optionsForAllRuns;
  {
    LuaThreadGroup lua(1);
    loadStrategy(lua, _cmdLineOptions, std::unique_ptr<imagesets::ImageSet>());
    Logger::Debug << "Loading options from strategy...\n";
    optionsForAllRuns =
        OptionsFunction::GetOptions(lua.GetThread(0).State(), _cmdLineOptions);
  }  // Let threadgroup go out of scope
  if (optionsForAllRuns.empty())
    optionsForAllRuns.emplace("main", _cmdLineOptions);
  for (const std::pair<const std::string, Options>& singleRunOptions :
       optionsForAllRuns) {
    Logger::Debug << "Starting run '" + singleRunOptions.first + "'...\n";
    run(singleRunOptions.second);
  }
}

void Runner::loadStrategy(
    LuaThreadGroup& lua, const Options& options,
    const std::unique_ptr<imagesets::ImageSet>& imageSet) {
  if (!options.preamble.empty()) {
    Logger::Debug << "Running preample...\n";
    lua.RunPreamble(options.preamble);
  }

  std::string executeFilename;
  // Execute filename override strategy filename, so that the options() function
  // can be in a different file than the execute functions.
  if (!options.executeFilename.empty())
    executeFilename = options.executeFilename;
  if (!options.strategyFilename.empty())
    executeFilename = options.strategyFilename;
  if (executeFilename.empty() && imageSet) {
    std::string telescopeName = imageSet->TelescopeName();
    TelescopeFile::TelescopeId telescopeId =
        TelescopeFile::TelescopeIdFromName(telescopeName);
    if (telescopeId == TelescopeFile::GENERIC_TELESCOPE) {
      Logger::Warn
          << "**\n"
             "** Measurement set specified the following telescope name: '"
          << telescopeName
          << "'\n"
             "** No good strategy is known for this telescope!\n"
             "** A generic strategy will be used which might not be optimal.\n"
             "**\n";
    } else {
      Logger::Info << "\nUsing a stock strategy that was optimized for the "
                      "following telescope:\n"
                   << "- " << TelescopeFile::TelescopeDescription(telescopeId)
                   << "\n"
                   << "Stock strategies might not perform well. It is "
                      "recommended to make your own strategy\n"
                   << "using the rfigui and specify it with the '-strategy "
                      "...' parameter.\n\n";
    }
    executeFilename = TelescopeFile::FindStrategy("", telescopeId);
    if (executeFilename.empty()) {
      throw std::runtime_error("Could not find a strategy for telescope " +
                               TelescopeFile::TelescopeName(telescopeId) +
                               ".\n"
                               "This strategy should have been installed when "
                               "running 'make install'. Make sure\n"
                               "aoflagger is properly installed.");
    }
  }
  if (!executeFilename.empty()) {
    try {
      Logger::Debug << "Opening strategy file '" << executeFilename << "'\n";
      lua.LoadFile(executeFilename.c_str());
      Logger::Debug << "Strategy parsed succesfully.\n";
    } catch (std::exception& e) {
      throw std::runtime_error("ERROR: Reading strategy file \"" +
                               executeFilename +
                               "\" failed!\n"
                               "\nThe thrown exception was:\n" +
                               e.what() + "\n");
    }
  }
}

static std::vector<std::string> FilterProcessedFiles(
    const std::vector<std::string>& ms_names) {
  std::vector<std::string> result;
  std::copy_if(ms_names.begin(), ms_names.end(), std::back_inserter(result),
               [](const std::string& ms_name) {
                 MSMetaData ms(ms_name);
                 if (!ms.HasAOFlaggerHistory()) {
                   return true;
                 }
                 Logger::Info
                     << "Skipping " << ms_name
                     << ",\n"
                        "because the set contains AOFlagger history and "
                        "-skip-flagged was given.\n";
                 return false;
               });
  return result;
}

void Runner::run(const Options& options) {
  Logger::SetVerbosity(options.logVerbosity.value_or(Logger::NormalVerbosity));

  size_t threadCount = options.CalculateThreadCount();
  Logger::Debug << "Number of threads: " << options.threadCount << "\n";

  const std::vector<std::string>& ms_files =
      options.skipFlagged ? FilterProcessedFiles(_cmdLineOptions.filenames)
                          : _cmdLineOptions.filenames;

  if (_cmdLineOptions.concatenateFrequency && ms_files.size() > 1) {
    // Only use the multi-band image set when there at least 2 files.
    // Else just use the simpler code.
    processFrequencyConcatenatedFiles(options, ms_files, threadCount);
  } else {
    for (const std::string& filename : ms_files) {
      processFile(options, filename, threadCount);
    }
  }
}

std::unique_ptr<ImageSet> Runner::initializeImageSet(const Options& options,
                                                     FileOptions& fileOptions) {
  MSOptions msOptions;
  msOptions.ioMode = options.readMode.value_or(BaselineIOMode::AutoReadMode);
  msOptions.baselineIntegration = options.baselineIntegration;

  std::unique_ptr<ImageSet> imageSet(ImageSet::Create(
      std::vector<std::string>{fileOptions.filename}, msOptions));
  bool isMS = dynamic_cast<MSImageSet*>(imageSet.get()) != nullptr;
  if (isMS) {
    MSImageSet* msImageSet = static_cast<MSImageSet*>(imageSet.get());
    if (options.dataColumn.empty())
      msImageSet->SetDataColumnName("DATA");
    else
      msImageSet->SetDataColumnName(options.dataColumn);
    msImageSet->SetReadUVW(options.readUVW.value_or(false));
    // during the first iteration, the nr of intervals hasn't been calculated
    // yet. Do that now.
    if (fileOptions.intervalIndex == 0) {
      if (options.chunkSize != 0) {
        msImageSet->SetInterval(fileOptions.intervalStart,
                                fileOptions.intervalEnd);
        const size_t obsTimesSize =
            msImageSet->MetaData().GetObservationTimes().size();
        fileOptions.nIntervals =
            (obsTimesSize + options.chunkSize - 1) / options.chunkSize;
        Logger::Info << "Maximum interval size of " << options.chunkSize
                     << " timesteps for total of " << obsTimesSize
                     << " timesteps results in " << fileOptions.nIntervals
                     << " intervals.\n";
        if (options.startTimestep)
          fileOptions.resolvedIntStart = *options.startTimestep;
        else
          fileOptions.resolvedIntStart = 0;
        if (options.endTimestep)
          fileOptions.resolvedIntEnd = *options.endTimestep;
        else
          fileOptions.resolvedIntEnd =
              obsTimesSize + fileOptions.resolvedIntStart;
      } else {
        fileOptions.nIntervals = 1;
      }
    }
    if (fileOptions.nIntervals == 1)
      msImageSet->SetInterval(fileOptions.intervalStart,
                              fileOptions.intervalEnd);
    else {
      size_t nTimes = fileOptions.resolvedIntEnd - fileOptions.resolvedIntStart;
      size_t start = fileOptions.resolvedIntStart + fileOptions.intervalIndex *
                                                        nTimes /
                                                        fileOptions.nIntervals;
      size_t end =
          fileOptions.resolvedIntStart +
          (fileOptions.intervalIndex + 1) * nTimes / fileOptions.nIntervals;
      Logger::Info << "Starting flagging of interval "
                   << fileOptions.intervalIndex << ", timesteps " << start
                   << " - " << end << '\n';
      msImageSet->SetInterval(start, end);
    }
    if (options.combineSPWs) {
      msImageSet->Initialize();
      imageSet.release();
      std::unique_ptr<MSImageSet> msImageSetPtr(msImageSet);
      imageSet.reset(new JoinedSPWSet(std::move(msImageSetPtr)));
    }
  }
  imageSet->Initialize();
  return imageSet;
}

void Runner::processFile(const Options& options, const std::string& filename,
                         size_t threadCount) {
  Logger::Info << "Starting strategy on "
               << to_simple_string(
                      boost::posix_time::microsec_clock::local_time())
               << '\n';

  ScriptData scriptData;
  FileOptions fileOptions;
  fileOptions.intervalStart = options.startTimestep;
  fileOptions.intervalEnd = options.endTimestep;
  fileOptions.filename = filename;
  bool isMS = false;
  while (fileOptions.intervalIndex < fileOptions.nIntervals) {
    std::unique_ptr<ImageSet> imageSet =
        initializeImageSet(options, fileOptions);
    isMS = dynamic_cast<MSImageSet*>(imageSet.get()) != nullptr;

    LuaThreadGroup lua(threadCount);

    loadStrategy(lua, options, imageSet);

    std::mutex ioMutex;
    BaselineIterator blIterator(&ioMutex, options);
    blIterator.Run(*imageSet, lua, scriptData);

    ++fileOptions.intervalIndex;
  }

  if (isMS) writeHistory(options, filename);

  finishStatistics(filename, scriptData, isMS);
}

struct ChunkInfo {
  size_t start_time_step;
  size_t end_time_step;
  size_t n_chunks;
  size_t chunk_size;
};

static std::optional<ChunkInfo> GetChunkInfo(const Options& options,
                                             const std::string& ms_name) {
  if (!options.chunkSize) return {};

  assert(options.startTimestep.has_value() == options.endTimestep.has_value() &&
         "These fields should either both be set or both be unset.");

  ChunkInfo result;
  if (options.startTimestep) {
    Logger::Info << "Interval " << *options.startTimestep << ", "
                 << *options.endTimestep << "\n";
    result.start_time_step = *options.startTimestep;
    result.end_time_step = *options.endTimestep;
  } else {
    MSMetaData MetaData{ms_name};
    result.start_time_step = 0;
    result.end_time_step = MetaData.TimestepCount();
  }

  const size_t time_step_count = result.end_time_step - result.start_time_step;
  result.n_chunks =
      (time_step_count + options.chunkSize - 1) / options.chunkSize;
  result.chunk_size = (time_step_count + result.n_chunks - 1) / result.n_chunks;

  Logger::Info << "Chunking settings result in " << time_step_count
               << " intervals with " << result.chunk_size << " timesteps.\n";

  if (result.n_chunks == 1) return {};

  return result;
}

void Runner::processFrequencyConcatenatedFiles(
    Options options, const std::vector<std::string>& ms_names,
    size_t n_threads) {
  Logger::Info << "Starting strategy on "
               << to_simple_string(
                      boost::posix_time::microsec_clock::local_time())
               << '\n';

  // Unlike processing threads increasing the number of IO threads isn't always
  // beneficial. At some point adding additional IO threads decreases the
  // performance. The optimal number of IO threads is system dependent. Testing
  // on different systems showed the optimum to be between 16 and 32 threads.
  // These tests were executed in February 2022.
  constexpr size_t kMaxIoThreads = 16;
  if (n_threads == 0) n_threads = aocommon::system::ProcessorCount();
  const size_t n_io_threads =
      std::min({kMaxIoThreads, n_threads, ms_names.size()});

  int remaining_chunks = 1;
  const std::optional<ChunkInfo> chunk_info =
      GetChunkInfo(options, ms_names.front());
  // When the -chunk-size argument is used and more than one chunk is used the
  // interval is adjusted.
  if (chunk_info) {
    remaining_chunks = chunk_info->n_chunks;
    options.startTimestep = chunk_info->start_time_step;
    options.endTimestep = *options.startTimestep + chunk_info->chunk_size;
  }

  // The number of chunks to process is greater than or equal to one.
  // After processing the next iteration is prepared. Therefore the termination
  // condition is in the middle of the loop.
  while (true) {
    if (chunk_info)
      Logger::Info << "Starting flagging of interval "
                   << 1 + (chunk_info->n_chunks - remaining_chunks)
                   << ", timesteps " << *options.startTimestep << " - "
                   << *options.endTimestep << '\n';

    ProcessFrequencyConcatenatedFilesChunk(options, ms_names, n_threads,
                                           n_io_threads, chunk_info);
    if (--remaining_chunks; remaining_chunks == 0) break;

    *options.startTimestep += chunk_info->chunk_size;
    options.endTimestep = remaining_chunks == 1
                              ? chunk_info->end_time_step
                              : *options.startTimestep + chunk_info->chunk_size;
  }

  for (const std::string& ms_name : ms_names) {
    writeHistory(options, ms_name);
  }
}

void Runner::ProcessFrequencyConcatenatedFilesChunk(
    const Options& options, const std::vector<std::string>& ms_names,
    size_t n_threads, size_t n_io_threads,
    const std::optional<ChunkInfo>& chunk_info) {
  std::unique_ptr<ImageSet> image_set =
      std::make_unique<imagesets::MultiBandMsImageSet>(
          ms_names, options.readMode.value_or(BaselineIOMode::AutoReadMode),
          options.startTimestep, options.endTimestep, n_io_threads);

  LuaThreadGroup thread_pool(n_threads);
  loadStrategy(thread_pool, options, image_set);

  std::mutex io_mutex;
  BaselineIterator baseline_iterator(&io_mutex, options);
  ScriptData script_data;
  baseline_iterator.Run(*image_set, thread_pool, script_data);

  static_cast<imagesets::MultiBandMsImageSet*>(image_set.get())
      ->WriteToMs(n_io_threads);

  if (script_data.GetStatistics())
    Logger::Warn
        << "Statistics can't be written in multi-MS processing mode.\n"
           "Please remove collecting statistics from your Lua strategy.\n";
}

void Runner::writeHistory(const Options& options, const std::string& filename) {
  MSMetaData ms(filename);
  Logger::Debug << "Adding strategy to history table of MS...\n";
  try {
    std::string strategyFilename;
    if (options.strategyFilename.empty())
      /// TODO
      ;
    else
      strategyFilename = options.strategyFilename;
    // std::ifstream strategyFile(strategyFilename);
    // std::string content((std::istreambuf_iterator<char>(strategyFile)),
    // (std::istreambuf_iterator<char>()) );
    ms.AddAOFlaggerHistory(strategyFilename, options.commandLine);
  } catch (std::exception& e) {
    Logger::Warn << "Failed to write history to MS: " << e.what() << '\n';
  }
}

void Runner::finishStatistics(const std::string& filename,
                              ScriptData& scriptData, bool isMS) {
  std::unique_ptr<StatisticsCollection>& statistics =
      scriptData.GetStatistics();
  if (statistics) {
    if (isMS) {
      Logger::Debug << "Writing quality statistics to MS.\n";
      QualityTablesFormatter qFormatter(filename);
      statistics->Save(qFormatter);
    } else {
      Logger::Warn
          << "Statistics can't be written when the input isn't a MS.\n"
             "Please remove collecting statistics from your Lua strategy.\n";
    }
  }
}
