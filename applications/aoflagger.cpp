#include "../aoluarunner/options.h"

#include "../structures/types.h"

#include "../util/logger.h"
#include "../util/stopwatch.h"
#include "../util/numberlist.h"

#include "../aoluarunner/runner.h"

#include <boost/algorithm/string/case_conv.hpp>

#include <version.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <mutex>

#define RETURN_SUCCESS 0
#define RETURN_CMDLINE_ERROR 10
#define RETURN_STRATEGY_PARSE_ERROR 20
#define RETURN_UNHANDLED_EXCEPTION 30

void checkRelease() {
#ifndef NDEBUG
  Logger::Warn << "This version of the AOFlagger has been compiled as DEBUG "
                  "version! (NDEBUG was not defined)\n"
               << "For better performance, recompile it as a RELEASE.\n\n";
#endif
}
void generalInfo() {
  Logger::Info << "AOFlagger " << AOFLAGGER_VERSION_STR << " ("
               << AOFLAGGER_VERSION_DATE_STR
               << ") command line application\n"
                  "Author: André Offringa (offringa@gmail.com)\n\n";
}

int main(int argc, char** argv) {
  if (argc == 1) {
    generalInfo();
    Logger::Error
        << "This program will execute a Lua flagging script that can be "
           "created with the RFI gui\n"
           "and executes it on one or several observations.\n\n"
           "Usage: "
        << argv[0]
        << " [options] <obs1> [<obs2> [..]]\n"
           "  -v will produce verbose output\n"
           "  -j overrides the number of threads specified in the strategy\n"
           "     (default: one thread for each CPU core)\n"
           "  -strategy <strategy>\n"
           "     specifies a customized strategy\n"
           "  -direct-read\n"
           "     Will perform the slowest IO but will always work.\n"
           "  -indirect-read\n"
           "     Will reorder the measurement set before starting, which is "
           "normally faster but requires\n"
           "     free disk space to reorder the data to.\n"
           "  -memory-read\n"
           "     Will read the entire measurement set in memory. This is the "
           "fastest, but requires much\n"
           "     memory.\n"
           "  -auto-read-mode\n"
           "     Will select either memory or direct mode based on available "
           "memory (default).\n"
           "  -skip-flagged\n"
           "     Will skip an ms if it has already been processed by AOFlagger "
           "according to its HISTORY\n"
           "     table.\n"
           "  -uvw\n"
           "     Reads uvw values (some exotic strategies require these)\n"
           "  -column <name>\n"
           "     Specify column to flag\n"
           "  -interval <start> <end>\n"
           "     Only process the specified timesteps. Indices are zero "
           "indexed, and\n"
           "     the end is exclusive, such that -interval 10 20 selects 10, "
           "11, ... 19.\n"
           "  -chunk-size <ntimes>\n"
           "     This will split the set into intervals with the given maximum "
           "size, and flag each\n"
           "     interval independently. This lowers the amount of memory "
           "required. The flagger\n"
           "     has slightly less information per interval, but for a size of "
           "1000 timesteps there is\n"
           "     no noticable difference. With a size of 100 the difference is "
           "mostly not problematic\n"
           "     either. In some cases, splitting the data increases accuracy, "
           "in particular when the\n"
           "     statistics in the set change significantly over time (e.g. "
           "rising Galaxy).\n"
           "  -bands <list>\n"
           "     Comma separated list of (zero-indexed) band ids to process\n"
           "  -fields <list>\n"
           "     Comma separated list of (zero-indexed) field ids to process\n"
           "  -baselines < all / cross / auto >\n"
           "     Run the strategy on the given baseline types. The default is "
           "to run the strategy on\n"
           "     all cross-correlation baselines. This parameter has no effect "
           "for single-dish observations.\n"
           "  -combine-spws\n"
           "     Join all SPWs together in frequency direction before "
           "flagging\n"
           "  -preamble <statement>\n"
           "     Runs the specified Lua statement before starting to flag. "
           "This is typically used to\n"
           "     define a variable, e.g. -preamble \"bandpassfile = "
           "mybandpass.txt\".\n"
           "\n"
           "This tool supports the Casacore measurement set, the SDFITS and "
           "Filterbank formats and some more. See\n"
           "the documentation for support of other file types.\n";

    checkRelease();

    return RETURN_CMDLINE_ERROR;
  }

  Options options;

  size_t parameterIndex = 1;
  while (parameterIndex < (size_t)argc && argv[parameterIndex][0] == '-') {
    std::string flag(argv[parameterIndex] + 1);

    // If "--" was used, strip another dash
    if (!flag.empty() && flag[0] == '-') flag = flag.substr(1);

    if (flag == "j" && parameterIndex < (size_t)(argc - 1)) {
      ++parameterIndex;
      options.threadCount = atoi(argv[parameterIndex]);
    } else if (flag == "v") {
      options.logVerbosity = Logger::VerboseVerbosity;
    } else if (flag == "version") {
      Logger::Info << "AOFlagger " << AOFLAGGER_VERSION_STR << " ("
                   << AOFLAGGER_VERSION_DATE_STR << ")\n";
      return 0;
    } else if (flag == "direct-read") {
      options.readMode = DirectReadMode;
    } else if (flag == "indirect-read") {
      options.readMode = IndirectReadMode;
    } else if (flag == "memory-read") {
      options.readMode = MemoryReadMode;
    } else if (flag == "auto-read-mode") {
      options.readMode = AutoReadMode;
    } else if (flag == "strategy") {
      parameterIndex++;
      options.strategyFilename = argv[parameterIndex];
    } else if (flag == "skip-flagged") {
      options.skipFlagged = true;
    } else if (flag == "uvw") {
      options.readUVW = true;
    } else if (flag == "column") {
      parameterIndex++;
      options.dataColumn = std::string(argv[parameterIndex]);
    } else if (flag == "bands") {
      ++parameterIndex;
      NumberList::ParseIntList(argv[parameterIndex], options.bands);
    } else if (flag == "fields") {
      ++parameterIndex;
      NumberList::ParseIntList(argv[parameterIndex], options.fields);
    } else if (flag == "combine-spws") {
      options.combineSPWs = true;
    } else if (flag == "concatenate-frequency") {
      options.concatenateFrequency = true;
    } else if (flag == "preamble") {
      ++parameterIndex;
      options.preamble.emplace_back(argv[parameterIndex]);
    } else if (flag == "interval") {
      options.startTimestep = atoi(argv[parameterIndex + 1]);
      options.endTimestep = atoi(argv[parameterIndex + 2]);
      parameterIndex += 2;
    } else if (flag == "chunk-size" || flag == "max-interval-size") {
      ++parameterIndex;
      options.chunkSize = atoi(argv[parameterIndex]);
    } else if (flag == "baselines") {
      ++parameterIndex;
      std::string bTypes = argv[parameterIndex];
      if (bTypes == "all")
        options.baselineSelection = BaselineSelection::All;
      else if (bTypes == "cross")
        options.baselineSelection = BaselineSelection::CrossCorrelations;
      else if (bTypes == "auto")
        options.baselineSelection = BaselineSelection::AutoCorrelations;
      else {
        Logger::Error << "Incorrect usage; baselines parameter should be set "
                         "to 'all', 'cross' or 'auto'.\n";
        return RETURN_CMDLINE_ERROR;
      }
    } else {
      Logger::Error << "Incorrect usage; parameter \"" << argv[parameterIndex]
                    << "\" not understood.\n";
      return RETURN_CMDLINE_ERROR;
    }
    ++parameterIndex;
  }

  try {
    Logger::SetVerbosity(
        options.logVerbosity.value_or(Logger::NormalVerbosity));
    generalInfo();

    checkRelease();

    Stopwatch watch(true);

    std::stringstream commandLineStr;
    commandLineStr << argv[0];
    for (int i = 1; i < argc; ++i) {
      commandLineStr << " \"" << argv[i] << '\"';
    }
    options.commandLine = commandLineStr.str();

    for (int i = parameterIndex; i < argc; ++i)
      options.filenames.emplace_back(argv[i]);

    std::filesystem::path strategyPath = options.strategyFilename;
    if (boost::to_lower_copy(strategyPath.extension().string()) == ".rfis") {
      Logger::Error << "An old .rfis file was specified. AOFlagger version 3 "
                       "supports only Lua scripts and can\n"
                       "not run the old .rfis-style files. Example Lua "
                       "strategies can be found in the aoflagger\n"
                       "source directory under data/strategies.\n";
      return RETURN_CMDLINE_ERROR;
    }

    Runner runner(options);
    runner.Run();

    Logger::Debug << "Time: " << watch.ToString() << "\n";

    return RETURN_SUCCESS;
  } catch (std::exception& exception) {
    std::cerr
        << "An unhandled exception occured: " << exception.what() << '\n'
        << "If you think this is a bug, please contact offringa@gmail.com\n";
    return RETURN_UNHANDLED_EXCEPTION;
  }
}
