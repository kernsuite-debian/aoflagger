#include "aoqplotcontroller.h"

#include "antennapagecontroller.h"
#include "baselinepagecontroller.h"
#include "blengthpagecontroller.h"
#include "frequencypagecontroller.h"
#include "tfpagecontroller.h"
#include "timepagecontroller.h"

#include "../aoqplotwindow.h"
#include "../baselineplotpage.h"
#include "../blengthplotpage.h"
#include "../frequencyplotpage.h"
#include "../histogrampage.h"
#include "../plotsheet.h"
#include "../summarypage.h"
#include "../timefrequencyplotpage.h"

#include "../../structures/measurementset.h"

#include "../../quality/histogramtablesformatter.h"
#include "../../quality/histogramcollection.h"
#include "../../quality/statisticscollection.h"

#include "../../remote/clusteredobservation.h"
#include "../../remote/processcommander.h"

AOQPlotController::AOQPlotController() :
	_isOpen(false),
	_window(nullptr)
{
}

AOQPlotController::~AOQPlotController()
{
}

void AOQPlotController::close()
{
	if(_isOpen)
	{
		_statCollection.reset();
		_histCollection.reset();
		_fullStats.reset();
		_isOpen = false;
	}
}

void AOQPlotController::readDistributedObservation(const std::string& filename, bool correctHistograms)
{
	std::unique_ptr<aoRemote::ClusteredObservation> observation(aoRemote::ClusteredObservation::Load(filename));
	_statCollection.reset(new StatisticsCollection());
	_histCollection.reset(new HistogramCollection());
	aoRemote::ProcessCommander commander(*observation);
	commander.PushReadAntennaTablesTask();
	commander.PushReadQualityTablesTask(_statCollection.get(), _histCollection.get(), correctHistograms);
	commander.Run();
	if(!commander.Errors().empty())
	{
		std::stringstream s;
		s << commander.Errors().size() << " error(s) occured while querying the nodes or measurement sets in the given observation. This might be caused by a failing node, an unreadable measurement set, or maybe the quality tables are not available. The errors reported are:\n\n";
		size_t count = 0;
		for(std::vector<std::string>::const_iterator i=commander.Errors().begin();i!=commander.Errors().end() && count < 30;++i)
		{
			s << "- " << *i << '\n';
			++count;
		}
		if(commander.Errors().size() > 30)
		{
			s << "... and " << (commander.Errors().size()-30) << " more.\n";
		}
		s << "\nThe program will continue, but this might mean that the statistics are incomplete. If this is the case, fix the issues and reopen the observation.";
		std::cerr << s.str() << std::endl;
		if(_window != nullptr)
		{
			_window->ShowError(s.str());
		}
	}
	
	_antennas = commander.Antennas();
}

void AOQPlotController::readMetaInfoFromMS(const string& filename)
{
	MeasurementSet ms(filename);
	_polarizationCount = ms.PolarizationCount();
	unsigned antennaCount = ms.AntennaCount();
	_antennas.clear();
	for(unsigned a=0;a<antennaCount;++a)
		_antennas.push_back(ms.GetAntennaInfo(a));
}

void AOQPlotController::readAndCombine(const string& filename)
{
	std::cout << "Adding " << filename << " to statistics...\n";
	QualityTablesFormatter qualityTables(filename);
	StatisticsCollection statCollection(_polarizationCount);
	statCollection.Load(qualityTables);
	_statCollection->Add(statCollection);
	
	HistogramTablesFormatter histogramTables(filename);
	if(histogramTables.HistogramsExist())
	{
		HistogramCollection histCollection(_polarizationCount);
		histCollection.Load(histogramTables);
		_histCollection->Add(histCollection);
	}
}

void AOQPlotController::ReadStatistics(const std::vector<std::string>& files, bool downsampleTime, bool downsampleFreq, size_t timeSize, size_t freqSize, bool correctHistograms)
{
	close();
	
	if(!files.empty())
	{
		const std::string& firstFile = *files.begin();
		if(aoRemote::ClusteredObservation::IsClusteredFilename(firstFile))
		{
			if(files.size() != 1)
				throw std::runtime_error("You are trying to open multiple distributed or clustered sets. Can only open multiple files if they are not distributed.");
			readDistributedObservation(firstFile, correctHistograms);
		}
		else {
			readMetaInfoFromMS(firstFile);
			
			_statCollection.reset(new StatisticsCollection(_polarizationCount));
			_histCollection.reset(new HistogramCollection(_polarizationCount));
			
			for(size_t i=0; i!=files.size(); ++i)
			{
				std::cout << " (" << (i+1) << "/" << files.size() << ") ";
				readAndCombine(files[i]);
			}
		}
		if(_window != nullptr)
			_window->SetShowHistograms(!_histCollection->Empty());
		if(downsampleTime)
		{
			std::cout << "Lowering time resolution..." << std::endl;
			_statCollection->LowerTimeResolution(timeSize);
		}

		if(downsampleFreq)
		{
			std::cout << "Lowering frequency resolution..." << std::endl;
			_statCollection->LowerFrequencyResolution(freqSize);
		}

		std::cout << "Integrating baseline statistics to one channel..." << std::endl;
		_statCollection->IntegrateBaselinesToOneChannel();
		
		std::cout << "Regridding time statistics..." << std::endl;
		_statCollection->RegridTime();
		
		std::cout << "Copying statistics..." << std::endl;
		_fullStats.reset(new StatisticsCollection(*_statCollection));
		
		std::cout << "Integrating time statistics to one channel..." << std::endl;
		_statCollection->IntegrateTimeToOneChannel();
		
		std::cout << "Opening statistics panel..." << std::endl;
		_isOpen = true;
	}
}

void AOQPlotController::Save(const AOQPlotController::PlotSavingData& data, size_t width, size_t height)
{
	const std::string& prefix = data.filenamePrefix;
	QualityTablesFormatter::StatisticKind kind = data.statisticKind;
	
	std::cout << "Saving " << prefix << "-antennas.pdf...\n";
	AntennaePageController antController;
	antController.SetStatistics(_statCollection.get(), _antennas);
	antController.SavePdf(prefix+"-antennas.pdf", kind);
	
	std::cout << "Saving " << prefix << "-baselines.pdf...\n";
	BaselinePageController baselController;
	BaselinePlotPage baselPage(&baselController);
	baselController.SetStatistics(_statCollection.get(), _antennas);
	baselController.SavePdf(prefix+"-baselines.pdf", kind, width, height);
	
	std::cout << "Saving " << prefix << "-baselinelengths.pdf...\n";
	BLengthPageController blenController;
	blenController.SetStatistics(_statCollection.get(), _antennas);
	blenController.SavePdf(prefix+"-baselinelengths.pdf", kind);
	
	std::cout << "Saving " << prefix << "-timefrequency.pdf...\n";
	TFPageController tfController;
	tfController.SetStatistics(_fullStats.get(), _antennas);
	tfController.SavePdf(prefix+"-timefrequency.pdf", kind, width, height);
	
	std::cout << "Saving " << prefix << "-time.pdf...\n";
	TimePageController timeController;
	timeController.SetStatistics(_statCollection.get(), _antennas);
	timeController.SavePdf(prefix+"-time.pdf", kind);
	
	std::cout << "Saving " << prefix << "-frequency.pdf...\n";
	FrequencyPageController freqController;
	freqController.SetStatistics(_statCollection.get(), _antennas);
	freqController.SavePdf(prefix+"-frequency.pdf", kind);
}

void AOQPlotController::Initialize(AOQPageController* controller, bool averagedStats)
{
	if(averagedStats)
		controller->SetStatistics(_statCollection.get(), _antennas);
	else
		controller->SetStatistics(_fullStats.get(), _antennas);
	controller->SetHistograms(_histCollection.get());
}

