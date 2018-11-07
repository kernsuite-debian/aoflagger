#include "aoqplot/aoqplotwindow.h"

#include "aoqplot/controllers/aoqplotcontroller.h"

#include <gtkmm/main.h>
#include <gtkmm/filechooserdialog.h>

#include "version.h"

#include "util/logger.h"

bool SelectFile(AOQPlotWindow& window, std::string& filename)
{
	Gtk::FileChooserDialog fileDialog(window, "Open observation set");
	
	fileDialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	fileDialog.add_button("_Open", Gtk::RESPONSE_OK);
	
	Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
	filter->set_name("Observation sets (*.{vds,gds,ref,MS})");
	filter->add_pattern("*.vds");
	filter->add_pattern("*.gds");
	filter->add_pattern("*.gvds");
	filter->add_pattern("*.ref");
	filter->add_pattern("*.MS");
	fileDialog.add_filter(filter);
	
	if(fileDialog.run() == Gtk::RESPONSE_OK)
	{
		filename = fileDialog.get_filename();
		return true;
	}
	else return false;
}

int main(int argc, char *argv[])
{
	// We have to 'lie' about argc to create(..), because of a bug in older gtkmms.
	int altArgc = 1;
	AOQPlotController controller;
	std::unique_ptr<AOQPlotWindow> window;
	bool openGUI = true;
	int argi = 1;
	std::vector<AOQPlotController::PlotSavingData> savedPlots;
	std::vector<std::string> files;
	while(argi < argc)
	{
		if(argv[argi][0]=='-')
		{
			std::string p;
			if(argv[argi][1] == '-')
				p = &argv[argi][2];
			else
				p = &argv[argi][1];
			if(p=="help" || p=="h")
			{
				Logger::Info << "Syntax: aoqplot [<options>] [<observation>]\n\n"
					"<observation> can be a measurement set for opening a single observation.\n"
					"To get statistics for a (remote) observation consisting of multiple measurement\n"
					"sets, specify a measurement set specifier instead (generally a .ref, .vds\n"
					".gvds or .gds file).\n"
					"\n"
					"Options can be:\n"
					"-help\n"
					"  Show syntax help.\n"
					"-version\n"
					"  Print version info and exit.\n"
					"-v\n"
					"  Verbose logging.\n"
					"-save [filename prefix] [statistic name]\n"
					"  Save every plot for the given kind of statistic as a PDF file. This\n"
					"  will prevent the GUI from opening. You can repeat this parameter to save\n"
					"  multiple kinds at once. A list of allowed names can be retrieved with\n"
					"  'aoquality liststats'. Some common ones are: StandardDeviation, Variance, Mean,\n"
					"  RFIPercentage, RFIRatio, Count.\n"
					"\n"
					"AOQPlot is part of the AOFlagger software package, written by André Offringa\n"
					"  (offringa@gmail.com). This AOQPlot belongs to AOFlagger " << AOFLAGGER_VERSION_STR << " (" << AOFLAGGER_VERSION_DATE_STR << ")\n";
				return 0;
			}
			else if(p=="save")
			{
				AOQPlotController::PlotSavingData newPlot;
				newPlot.filenamePrefix = argv[argi+1];
				newPlot.statisticKind = QualityTablesFormatter::NameToKind(argv[argi+2]);
				argi += 2;
				openGUI = false;
				savedPlots.push_back(newPlot);
			}
			else if(p == "version")
			{
				Logger::Info << "AOQplot " << AOFLAGGER_VERSION_STR << " (" << AOFLAGGER_VERSION_DATE_STR << ")\n";
				return 0;
			}
			else if(p == "v")
			{
				Logger::SetVerbosity(Logger::VerboseVerbosity);
			}
			else {
				Logger::Error << "Bad parameter specified: " << argv[argi] << '\n';
				return 1;
			}
		}
		else {
			files.push_back(argv[argi]);
		}
		++argi;
	}
	
	if(openGUI)
	{
		Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(altArgc, argv, "", Gio::APPLICATION_HANDLES_OPEN);
		window.reset(new AOQPlotWindow(&controller));
		window->show_all();
		if(files.empty())
		{
			std::string filename;
			if(SelectFile(*window, filename))
				files.push_back(filename);
			else
				return 0;
		}
		window->Open(files);
		app->run(*window);
	}
	else {
		if(files.empty())
		{
			Logger::Error << "No observation specified.\n";
			return 1;
		}
		
		controller.ReadStatistics(files);
	
		for(const AOQPlotController::PlotSavingData& plot : savedPlots)
		{
			controller.Save(plot, 640, 480);
		}
	}
	
	return 0;
}
