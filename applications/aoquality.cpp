#include <iostream>

#include "../util/plot.h"

#include "../quality/operations.h"

#include <optional>

namespace {

std::vector<std::string> AsVector(char* argv[], size_t start, size_t argc) {
  std::vector<std::string> result;
  for (size_t i = start; i != argc; ++i) result.emplace_back(argv[i]);
  return result;
}

void printSyntax(std::ostream& stream, char* argv[]) {
  stream << "Syntax: " << argv[0]
         << " <action> [options]\n\n"
            "Possible actions:\n"
            "\thelp        - Get more info about an action (usage: '"
         << argv[0]
         << " help <action>')\n"
            "\tcollect     - Processes the entire measurement set, collects "
            "the statistics\n"
            "\t              and writes them in the quality tables.\n"
            "\tcombine     - Combine several tables.\n"
            "\thistogram   - Various histogram actions.\n"
            "\tliststats   - Display a list of possible statistic kinds.\n"
            "\tquery_a     - Query per antenna.\n"
            "\tquery_b     - Query per baseline.\n"
            "\tquery_t     - Query per time step.\n"
            "\tquery_f     - Query per frequency.\n"
            "\tquery_fr    - Query a frequency range\n"
            "\tquery_g     - Query single global statistic.\n"
            "\tremove      - Remove all quality tables.\n"
            "\tsummarize   - Give a summary of the statistics currently in the "
            "quality tables.\n"
            "\tsummarizerfi- Give a summary of the rfi statistics.\n"
            "\n\n"
            "A few actions take a statistic kind. Some common statistic kinds "
            "are: StandardDeviation,\n"
            "Variance, Mean, RFIPercentage, RFIRatio, Count. These are case "
            "sensitive. Run 'aoquality liststats' for a full list.\n";
}

void HistogramAction(const std::string& filename, const std::string& query,
                     const char* dataColumnName) {
  if (query == "rfislope") {
    quality::PrintRfiSlope(filename);
  } else if (query == "rfislope-per-baseline") {
    quality::PrintRfiSlopePerBaseline(filename, dataColumnName);
  } else if (query == "remove") {
    quality::RemoveHistogram(filename);
  } else {
    std::cerr << "Unknown histogram command: " << query << "\n";
  }
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef HAS_LOFARSTMAN
  register_lofarstman();
#endif  // HAS_LOFARSTMAN

  if (argc < 2) {
    printSyntax(std::cerr, argv);
    return -1;
  } else {
    const std::string action = argv[1];

    if (action == "help") {
      if (argc != 3) {
        printSyntax(std::cout, argv);
      } else {
        const std::string helpAction = argv[2];
        if (helpAction == "help") {
          printSyntax(std::cout, argv);
        } else if (helpAction == "collect") {
          std::cout
              << "Syntax: " << argv[0]
              << " collect [-d [column]/-tf/-h] <ms> [quack timesteps] [list "
                 "of antennae]\n\n"
                 "The collect action will go over a whole measurement set and "
                 "\n"
                 "collect the default statistics. It will write the results in "
                 "the \n"
                 "quality subtables of the main measurement set.\n\n"
                 "Currently, the default statistics are:\n"
                 "\tRFIRatio, Count, Mean, SumP2, DCount, DMean, DSumP2.\n"
                 "The subtables that will be updated are:\n"
                 "\tQUALITY_KIND_NAME, QUALITY_TIME_STATISTIC,\n"
                 "\tQUALITY_FREQUENCY_STATISTIC and "
                 "QUALITY_BASELINE_STATISTIC.\n\n";
        } else if (helpAction == "summarize") {
          std::cout
              << "Syntax: " << argv[0]
              << " summarize <ms>\n\n"
                 "Gives a summary of the statistics in the measurement set.\n";
        } else if (helpAction == "query_a") {
          std::cout << "Syntax: " << argv[0]
                    << " query_a <kind> <ms>\n\n"
                       "Prints the given statistic for each antenna.\n";
        } else if (helpAction == "query_b") {
          std::cout << "Syntax: " << argv[0]
                    << " query_b <kind> <ms>\n\n"
                       "Prints the given statistic for each baseline.\n";
        } else if (helpAction == "query_t") {
          std::cout << "Syntax: " << argv[0]
                    << " query_t <kind> <ms>\n\n"
                       "Print the given statistic for each time step.\n";
        } else if (helpAction == "query_f") {
          std::cout << "Syntax: " << argv[0]
                    << " query_f <kind> <ms>\n\n"
                       "Print the given statistic for each frequency.\n";
        } else if (helpAction == "query_g") {
          std::cout << "Syntax " << argv[0]
                    << " query_g <kind> <ms>\n\n"
                       "Print the given statistic for this measurement set.\n";
        } else if (helpAction == "combine") {
          std::cout << "Syntax: " << argv[0]
                    << " combine <target_ms> [<in_ms> [<in_ms> ..]]\n\n"
                       "This will read all given input measurement sets, "
                       "combine the statistics and \n"
                       "write the results to a target measurement set. The "
                       "target measurement set should\n"
                       "not exist beforehand.\n";
        } else if (helpAction == "histogram") {
          std::cout << "Syntax: " << argv[0]
                    << " histogram <query> <ms>]\n\n"
                       "Query can be:\n"
                       "\trfislope - performs linear regression on the part of "
                       "the histogram that should contain the RFI.\n"
                       "\t           Reports one value per polarisation.\n";
        } else if (helpAction == "remove") {
          std::cout << "Syntax: " << argv[0]
                    << " remove [ms]\n\n"
                       "This will completely remove all quality tables from "
                       "the measurement set.\n";
        } else {
          std::cerr << "Unknown action specified in help.\n";
          return -1;
        }
      }
    } else if (action == "liststats") {
      quality::ListStatistics();
    } else if (action == "collect") {
      if (argc < 3) {
        std::cerr << "collect actions needs one or two parameters (the "
                     "measurement set)\n";
        return -1;
      } else {
        int argi = 2;
        bool histograms = false, timeFrequency = false;
        const char* dataColumnName = "DATA";
        size_t intervalStart = 0, intervalEnd = 0;
        while (argi < argc && argv[argi][0] == '-') {
          const std::string p = &argv[argi][1];
          if (p == "h") {
            histograms = true;
          } else if (p == "d") {
            ++argi;
            dataColumnName = argv[argi];
          } else if (p == "tf") {
            timeFrequency = true;
          } else if (p == "interval") {
            intervalStart = atoi(argv[argi + 1]);
            intervalEnd = atoi(argv[argi + 2]);
            argi += 2;
          } else {
            throw std::runtime_error(
                "Bad parameter given to aoquality collect");
          }
          ++argi;
        }
        const std::string filename = argv[argi];
        size_t flaggedTimesteps = 0;
        ++argi;
        std::set<size_t> flaggedAntennae;
        if (argi != argc) {
          flaggedTimesteps = atoi(argv[argi]);
          ++argi;
          while (argi != argc) {
            flaggedAntennae.insert(atoi(argv[argi]));
            ++argi;
          }
        }
        Collector::CollectingMode mode;
        if (histograms)
          mode = Collector::CollectHistograms;
        else if (timeFrequency)
          mode = Collector::CollectTimeFrequency;
        else
          mode = Collector::CollectDefault;
        quality::CollectStatistics(filename, mode, flaggedTimesteps,
                                   std::move(flaggedAntennae), dataColumnName,
                                   intervalStart, intervalEnd);
      }
    } else if (action == "combine") {
      if (argc < 3) {
        std::cerr << "combine actions needs at least one parameter: aoquality "
                     "combine <output> <input1> [<input2> ...]\n";
        return -1;
      } else {
        const std::string outFilename = argv[2];
        quality::CombineStatistics(outFilename, AsVector(argv, 3, argc));
      }
    } else if (action == "histogram") {
      if (argc < 4) {
        std::cerr
            << "histogram actions needs at least two parameters (the query and "
               "the measurement set)\n";
        return -1;
      } else {
        HistogramAction(argv[3], argv[2], "DATA");
      }
    } else if (action == "summarize") {
      if (argc < 3) {
        std::cerr << "summarize actions needs at least one parameter (the "
                     "measurement set)\n";
        return -1;
      } else {
        quality::PrintSummary(AsVector(argv, 2, argc));
      }
    } else if (action == "summarizerfi") {
      if (argc < 3) {
        std::cerr << "summarizerfi actions needs at least one parameter (the "
                     "measurement set)\n";
        return -1;
      } else {
        quality::PrintRfiSummary(AsVector(argv, 2, argc));
      }
    } else if (action == "query_g") {
      if (argc < 4) {
        std::cerr << "Syntax for query global stat: 'aoquality query_g <KIND> "
                     "<MS> [<MS2> ...]'\n";
        return -1;
      } else {
        quality::PrintGlobalStatistic(argv[2], AsVector(argv, 3, argc));
      }
    } else if (action == "query_a") {
      if (argc < 4) {
        std::cerr << "Syntax for query antennas: 'aoquality query_a <KIND> "
                     "<MS> [<MS2> ...]'\n";
        return -1;
      } else {
        quality::PrintPerAntennaStatistics(argv[2], AsVector(argv, 3, argc));
        return 0;
      }
    } else if (action == "query_b") {
      if (argc < 4) {
        std::cerr << "Syntax for query baselines: 'aoquality query_b <KIND> "
                     "<MS> [<MS2> ...]'\n";
        return -1;
      } else {
        quality::PrintPerBaselineStatistics(argv[2], AsVector(argv, 3, argc));
      }
    } else if (action == "query_f") {
      if (argc < 4) {
        std::cerr << "Syntax for query times: 'aoquality query_t [options] "
                     "<KIND> <MS1> [<MS2> ...]'\n"
                     "Options:\n"
                     "  -downsample <n_bins>\n"
                     "    Average down the statistics in frequency to the "
                     "given nr of bins.\n";
        return -1;
      } else {
        size_t argi = 2;
        std::optional<size_t> downsample;
        while (argv[argi][0] == '-') {
          const std::string p(&argv[argi][1]);
          if (p == "downsample") {
            ++argi;
            downsample = std::atoi(argv[argi]);
          } else {
            throw std::runtime_error("Invalid parameter: " + p);
          }
          ++argi;
        }
        quality::PrintPerFrequencyStatistics(
            argv[argi], AsVector(argv, argi + 1, argc), downsample);
        return 0;
      }
    } else if (action == "query_fr") {
      if (argc == 5) {
        const std::string range = argv[4];
        if (range == "DVB4") {
          quality::PrintFrequencyRangeStatistic(
              argv[2], AsVector(argv, 3, argc), 167, 174);
        } else if (range == "DVB5") {
          quality::PrintFrequencyRangeStatistic(
              argv[2], AsVector(argv, 3, argc), 174, 181);
        } else if (range == "DVB6") {
          quality::PrintFrequencyRangeStatistic(
              argv[2], AsVector(argv, 3, argc), 181, 188);
        } else if (range == "DVB7") {
          quality::PrintFrequencyRangeStatistic(
              argv[2], AsVector(argv, 3, argc), 188, 195);
        } else {
          std::cerr << "Syntax for query times: 'aoquality query_fr <KIND> "
                       "<MS> <START MHZ> <END MHZ>'\n";
          return -1;
        }
        return 0;
      } else if (argc == 6) {
        quality::PrintFrequencyRangeStatistic(argv[2], {argv[3]}, atof(argv[4]),
                                              atof(argv[5]));
        return 0;
      } else {
        std::cerr << "Syntax for query frequency range: 'aoquality query_fr "
                     "<KIND> <MS> "
                     "<START MHZ> <END MHZ>'\n";
        return -1;
      }
    } else if (action == "query_t") {
      if (argc < 4) {
        std::cerr << "Syntax for query times: 'aoquality query_t <KIND> <MS1> "
                     "[<MS2> ...]'\n";
        return -1;
      } else {
        quality::PrintPerTimeStatistics(argv[2], AsVector(argv, 3, argc));
        return 0;
      }
    } else if (action == "remove") {
      if (argc != 3) {
        std::cerr
            << "Syntax for removing quality tables: 'aoquality remove <MS>'\n";
        return -1;
      } else {
        quality::RemoveStatistics(argv[2]);
        return 0;
      }
    } else {
      std::cerr << "Unknown action '" << action << "'.\n\n";
      printSyntax(std::cerr, argv);
      return -1;
    }

    return 0;
  }
}
