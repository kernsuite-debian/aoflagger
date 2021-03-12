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

#include "../util/logger.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <fstream>

using namespace rfiStrategy;

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
    loadStrategy(lua, _cmdLineOptions,
                 std::unique_ptr<rfiStrategy::ImageSet>());
    Logger::Debug << "Loading options from strategy...\n";
    optionsForAllRuns =
        OptionsFunction::GetOptions(lua.GetThread(0).State(), _cmdLineOptions);
  }  // Let threadgroup go out of scope
  if (optionsForAllRuns.empty())
    optionsForAllRuns.emplace("main", _cmdLineOptions);
  for (const std::pair<std::string, Options>& singleRunOptions :
       optionsForAllRuns) {
    Logger::Debug << "Starting run '" + singleRunOptions.first + "'...\n";
    run(singleRunOptions.second);
  }
}

void Runner::loadStrategy(
    LuaThreadGroup& lua, const Options& options,
    const std::unique_ptr<rfiStrategy::ImageSet>& imageSet) {
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

void Runner::run(const Options& options) {
  Logger::SetVerbosity(options.logVerbosity.value_or(Logger::NormalVerbosity));

  size_t threadCount = options.CalculateThreadCount();
  Logger::Debug << "Number of threads: " << options.threadCount << "\n";

  for (const std::string& filename : _cmdLineOptions.filenames) {
    processFile(options, filename, threadCount);
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
    msImageSet->SetReadUVW(options.readUVW.get_value_or(false));
    // during the first iteration, the nr of intervals hasn't been calculated
    // yet. Do that now.
    if (fileOptions.intervalIndex == 0) {
      if (options.chunkSize != 0) {
        msImageSet->SetInterval(fileOptions.intervalStart,
                                fileOptions.intervalEnd);
        const size_t obsTimesSize =
            msImageSet->MetaData().GetObservationTimesSet().size();
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

  bool skip = false;
  if (options.skipFlagged) {
    MSMetaData set(filename);
    if (set.HasAOFlaggerHistory()) {
      skip = true;
      Logger::Info << "Skipping " << filename
                   << ",\n"
                      "because the set contains AOFlagger history and "
                      "-skip-flagged was given.\n";
    }
  }

  if (!skip) {
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
      // TODO write separate .qs file?
    }
  }
}
