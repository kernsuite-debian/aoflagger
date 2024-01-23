#include "../rfigui/rfiguiwindow.h"

#include "../rfigui/controllers/imagecomparisoncontroller.h"
#include "../rfigui/controllers/rfiguicontroller.h"

#include "../util/logger.h"
#include "../util/progress/stdoutreporter.h"

#include "../imagesets/msimageset.h"
#include "../imagesets/msoptions.h"

#include <version.h>

#include <gtkmm/application.h>

#include <pangomm/init.h>

#include <glibmm/error.h>
#include <glibmm/wrap.h>

#include <string>
#include <vector>

#include <libgen.h>

struct SavedBaseline {
  size_t a1Index, a2Index, bandIndex, sequenceIndex;
  std::string filename;
  bool operator<(const SavedBaseline& rhs) const {
    return std::tie(a1Index, a2Index, bandIndex, sequenceIndex, filename) <
           std::tie(rhs.a1Index, rhs.a2Index, rhs.bandIndex, rhs.sequenceIndex,
                    rhs.filename);
  }
};

static void run(int argc, char* argv[]) {
  int argi = 1;

  std::vector<std::string> filenames;
  std::set<SavedBaseline> savedBaselines;
  bool interactive = true;
  std::string dataColumnName = "DATA";
  bool plotFlags = true;
  std::optional<size_t> intervalStart, intervalEnd;

  while (argi < argc) {
    if (argv[argi][0] == '-') {
      std::string p;
      if (argv[argi][1] == '-')
        p = &argv[argi][2];
      else
        p = &argv[argi][1];
      if (p == "h" || p == "help" || p == "?") {
        Logger::Info
            << "The RFIGui is a program to analyze the time-frequency "
               "information in radio astronomical observations.\n"
            << "It is written by André Offringa (offringa@gmail.com) and "
               "published under the GPL 3.\n"
            << "This program is part of AOFlagger " << AOFLAGGER_VERSION_STR
            << " (" << AOFLAGGER_VERSION_DATE_STR << ")\n\n"
            << "Syntax:\n"
            << "  rfigui [-option1 [-option2 ...]] [measurement set]\n\n"
            << "The main window will be opened when no parameters are "
               "specified.\n"
            << "Possible options:\n"
            << " -help\n"
            << "    Display this help message and exit.\n"
            << " -version\n"
            << "    Display AOFlagger version and exit.\n"
            << " -v\n"
            << "    Verbose logging.\n"
            << " -save-baseline <filename> <antenna1> <antenna2> <band> "
               "<sequence index>\n"
            << "    Save the selected baseline to the given filename. The "
               "parameter can be specified\n"
            << "    multiple times to save multiple baselines in one run. When "
               "this parameter is specified,\n"
            << "    the main window will not open and the program will exit "
               "after saving the requested baselines.\n"
            << " -data-column <columnname>\n"
            << "    Open the selected column name.\n"
            << " -hide-flags / -show-flags\n"
            << "    Do not (or do) plot the flag mask on top of the data. "
               "Default: plot them.\n";
        return;
      } else if (p == "version") {
        Logger::Info << "AOFlagger " << AOFLAGGER_VERSION_STR << " ("
                     << AOFLAGGER_VERSION_DATE_STR << ")\n";
        return;
      } else if (p == "save-baseline") {
        SavedBaseline sb;
        sb.filename = argv[argi + 1];
        sb.a1Index = atoi(argv[argi + 2]);
        sb.a2Index = atoi(argv[argi + 3]);
        sb.bandIndex = atoi(argv[argi + 4]);
        sb.sequenceIndex = atoi(argv[argi + 5]);
        savedBaselines.insert(sb);
        interactive = false;
        argi += 5;
      } else if (p == "data-column") {
        ++argi;
        dataColumnName = argv[argi];
      } else if (p == "hide-flags") {
        plotFlags = false;
      } else if (p == "show-flags") {
        plotFlags = true;
      } else if (p == "v") {
        Logger::SetVerbosity(Logger::VerboseVerbosity);
      } else {
        Logger::Error << "Unknown parameter " << argv[argi]
                      << " specified on command line.\n";
        return;
      }
    } else {
      filenames.push_back(argv[argi]);
    }
    ++argi;
  }

  // We have to 'lie' about argc to create(..), because of a bug in older
  // gtkmms.
  int altArgc = 1;
  RFIGuiController controller;
  Glib::RefPtr<Gtk::Application> app;
  std::unique_ptr<RFIGuiWindow> window;
  if (interactive) {
    app = Gtk::Application::create(altArgc, argv, "",
                                   Gio::APPLICATION_HANDLES_OPEN);
    window = std::make_unique<RFIGuiWindow>(&controller, argv[0]);
    window->present();
  }

  try {
    if (!filenames.empty()) {
      if (interactive) {
        window->OpenPaths(filenames);
      } else {
        Pango::init();
        MSOptions options;
        options.ioMode = DirectReadMode;
        options.dataColumnName = dataColumnName;
        options.combineSPWs = false;
        options.intervalStart = intervalStart;
        options.intervalEnd = intervalEnd;
        options.baselineIntegration.enable = false;
        controller.OpenMS(filenames, options);
      }
    }

    if (!savedBaselines.empty()) {
      imagesets::IndexableSet* imageSet =
          dynamic_cast<imagesets::IndexableSet*>(&controller.GetImageSet());
      if (imageSet == nullptr)
        throw std::runtime_error(
            "Option -save-baseline can only be used for measurement sets.\n");
      for (const SavedBaseline& b : savedBaselines) {
        auto index =
            imageSet->Index(b.a1Index, b.a2Index, b.bandIndex, b.sequenceIndex);
        if (!index) throw std::runtime_error("Baseline not found!");
        controller.SetImageSetIndex(*index);
        StdOutReporter reporter;
        controller.LoadCurrentTFDataAsync(reporter);
        controller.LoadCurrentTFDataFinish(true);
        MaskedHeatMap& plot = controller.TFController().Plot();
        plot.SetShowOriginalMask(plotFlags);
        plot.SetShowXAxisDescription(true);
        plot.SetShowYAxisDescription(true);
        plot.SetShowZAxisDescription(true);
        plot.SaveByExtension(b.filename, 800, 480);
      }
    }

    if (interactive) app->run(*window);
  } catch (const std::exception& e) {
    Logger::Error << "\n"
                     "==========\n"
                     "An unhandled exception occured while executing RFIGui. "
                     "The error is:\n"
                  << e.what() << '\n';
  }
}

int main(int argc, char* argv[]) {
  run(argc, argv);

  Glib::Error::register_cleanup();
  Glib::wrap_register_cleanup();

  return 0;
}
