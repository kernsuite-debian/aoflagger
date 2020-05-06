#include <iostream>
#include <string>
#include <mutex>

#include "strategy/actions/foreachmsaction.h"
#include "strategy/actions/strategy.h"

#include "strategy/algorithms/baselineselector.h"
#include "strategy/algorithms/polarizationstatistics.h"

#include "strategy/plots/antennaflagcountplot.h"
#include "strategy/plots/frequencyflagcountplot.h"
#include "strategy/plots/timeflagcountplot.h"

#include "strategy/control/artifactset.h"
#include "strategy/control/strategyreader.h"
#include "strategy/control/defaultstrategy.h"

#include "structures/system.h"

#include "util/logger.h"
#include "util/progresslistener.h"
#include "util/stopwatch.h"
#include "util/numberlist.h"

#include "version.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional/optional.hpp>

class ConsoleProgressHandler : public ProgressListener {
	private:
		std::mutex _mutex;
		bool _exceptionOccurred;
		
	public:
		ConsoleProgressHandler() : _exceptionOccurred(false) { }
		
		bool ExceptionOccurred() const { return _exceptionOccurred; }
		
		virtual void OnStartTask(const rfiStrategy::Action &action, size_t taskNo, size_t taskCount, const std::string &description, size_t weight) final override
		{
			std::lock_guard<std::mutex> lock(_mutex);
			ProgressListener::OnStartTask(action, taskNo, taskCount, description, weight);
			
			double totalProgress = TotalProgress();
			
			Logger::Progress << round(totalProgress*1000.0)/10.0 << "% : ";
			
			for(size_t i=1;i<Depth();++i)
				Logger::Progress << "+-";
			
			Logger::Progress << description << "...\n";
		}
		
		virtual void OnEndTask(const rfiStrategy::Action &action) final override
		{
			std::lock_guard<std::mutex> lock(_mutex);
			
			ProgressListener::OnEndTask(action);
		}

		virtual void OnProgress(const rfiStrategy::Action &action, size_t i, size_t j) final override
		{
			ProgressListener::OnProgress(action, i, j);
		}

		virtual void OnException(const rfiStrategy::Action &, std::exception &thrownException) final override
		{
			std::lock_guard<std::mutex> lock(_mutex);
			Logger::Error <<
				"An exception occured during execution of the strategy!\n"
				"Your set might not be fully flagged. Exception was:\n"
				<< thrownException.what() << '\n';
			_exceptionOccurred = true;
		}
};

#define RETURN_SUCCESS                0
#define RETURN_CMDLINE_ERROR         10
#define RETURN_STRATEGY_PARSE_ERROR  20
#define RETURN_UNHANDLED_EXCEPTION   30

void checkRelease()
{
#ifndef NDEBUG
		Logger::Warn
			<< "This version of the AOFlagger has been compiled as DEBUG version! (NDEBUG was not defined)\n"
			<< "For better performance, recompile it as a RELEASE.\n\n";
#endif
}
void generalInfo()
{
	Logger::Info << 
		"AOFlagger " << AOFLAGGER_VERSION_STR << " (" << AOFLAGGER_VERSION_DATE_STR <<
		") command line application\n"
		"This program will execute an RFI strategy as can be created with the RFI gui\n"
		"and executes it on one or several observations.\n\n"
		"Author: André Offringa (offringa@gmail.com)\n\n";
}

int main(int argc, char **argv)
{
	if(argc == 1)
	{
		generalInfo();
		Logger::Error << "Usage: " << argv[0] << " [options] <obs1> [<obs2> [..]]\n"
		"  -v will produce verbose output\n"
		"  -j overrides the number of threads specified in the strategy\n"
		"     (default: one thread for each CPU core)\n"
		"  -strategy <strategy>\n"
		"     specifies a customized strategy\n"
		"  -direct-read\n"
		"     Will perform the slowest IO but will always work.\n"
		"  -indirect-read\n"
		"     Will reorder the measurement set before starting, which is normally faster but requires\n"
		"     free disk space to reorder the data to.\n"
		"  -memory-read\n"
		"     Will read the entire measurement set in memory. This is the fastest, but requires much\n"
		"     memory.\n"
		"  -auto-read-mode\n"
		"     Will select either memory or direct mode based on available memory (default).\n"
		"  -skip-flagged\n"
		"     Will skip an ms if it has already been processed by AOFlagger according to its HISTORY\n"
		"     table.\n"
		"  -uvw\n"
		"     Reads uvw values (some exotic strategies require these)\n"
		"  -column <name>\n"
		"     Specify column to flag\n"
		"  -interval <start> <end>\n"
		"     Only process the specified timesteps. Indices are zero indexed, and\n"
		"     the end is exclusive, such that -interval 10 20 selects 10, 11, ... 19.\n"
		"  -max-interval-size <ntimes>\n"
		"     This will split the set into intervals with the given maximum size, and flag each\n"
		"     interval independently. This lowers the amount of memory required. The flagger\n"
		"     has slightly less information per interval, but for a size of 1000 timesteps there is\n"
		"     no noticable difference. With a size of 100 the difference is mostly not problematic\n"
		"     either. In some cases, splitting the data increases accuracy, in particular when the\n"
		"     statistics in the set change significantly over time (e.g. rising Galaxy).\n"
		"  -bands <list>\n"
		"     Comma separated list of (zero-indexed) band ids to process\n"
		"  -fields <list>\n"
		"     Comma separated list of (zero-indexed) field ids to process\n"
		"  -combine-spws\n"
		"     Join all SPWs together in frequency direction before flagging\n"
		"  -bandpass <filename>\n"
		"     Set bandpass correction file for any 'Apply passband' action\n"
		"\n"
		"This tool supports the Casacore measurement set, the SDFITS and Filterbank formats and some more. See\n"
		"the documentation for support of other file types.\n";
		
		checkRelease();
		
		return RETURN_CMDLINE_ERROR;
	}
	
	boost::optional<size_t> threadCount;
	boost::optional<BaselineIOMode> readMode;
	boost::optional<bool> readUVW;
	boost::optional<std::string> strategyFile;
	boost::optional<Logger::VerbosityLevel> logVerbosity;
	boost::optional<bool> skipFlagged;
	boost::optional<std::string> dataColumn;
	boost::optional<std::pair<size_t, size_t>> interval;
	boost::optional<size_t> maxIntervalSize;
	boost::optional<bool> combineSPWs;
	boost::optional<std::string> bandpass;
	std::set<size_t> bands, fields;

	size_t parameterIndex = 1;
	while(parameterIndex < (size_t) argc && argv[parameterIndex][0]=='-')
	{
		std::string flag(argv[parameterIndex]+1);
		
		// If "--" was used, strip another dash
		if(!flag.empty() && flag[0] == '-')
			flag = flag.substr(1);
		
		if(flag=="j" && parameterIndex < (size_t) (argc-1))
		{
			++parameterIndex;
			threadCount = atoi(argv[parameterIndex]);
		}
		else if(flag=="v")
		{
			logVerbosity = Logger::VerboseVerbosity;
		}
		else if(flag == "version")
		{
			Logger::Info << "AOFlagger " << AOFLAGGER_VERSION_STR << " (" << AOFLAGGER_VERSION_DATE_STR << ")\n";
			return 0;
		}
		else if(flag=="direct-read")
		{
			readMode = DirectReadMode;
		}
		else if(flag=="indirect-read")
		{
			readMode = IndirectReadMode;
		}
		else if(flag=="memory-read")
		{
			readMode = MemoryReadMode;
		}
		else if(flag=="auto-read-mode")
		{
			readMode = AutoReadMode;
		}
		else if(flag=="strategy")
		{
			parameterIndex++;
			strategyFile = argv[parameterIndex];
		}
		else if(flag=="skip-flagged")
		{
			skipFlagged = true;
		}
		else if(flag=="uvw")
		{
			readUVW = true;
		}
		else if(flag == "column")
		{
			parameterIndex++;
			dataColumn = std::string(argv[parameterIndex]); 
		}
		else if(flag == "bands")
		{
			++parameterIndex;
			NumberList::ParseIntList(argv[parameterIndex], bands);
		}
		else if(flag == "fields")
		{
			++parameterIndex;
			NumberList::ParseIntList(argv[parameterIndex], fields);
		}
		else if(flag == "combine-spws")
		{
			combineSPWs = true;
		}
		else if(flag == "bandpass")
		{
			++parameterIndex;
			bandpass = std::string(argv[parameterIndex]);
		}
		else if(flag == "interval")
		{
			interval = std::pair<size_t,size_t>(
				atoi(argv[parameterIndex+1]),
				atoi(argv[parameterIndex+2]));
			parameterIndex += 2;
		}
		else if(flag == "max-interval-size")
		{
			++parameterIndex;
			maxIntervalSize = atoi(argv[parameterIndex]);
		}
		else
		{
			Logger::Error << "Incorrect usage; parameter \"" << argv[parameterIndex] << "\" not understood.\n";
			return 1;
		}
		++parameterIndex;
	}

	try {
		Logger::SetVerbosity(logVerbosity.value_or(Logger::NormalVerbosity));
		generalInfo();
			
		checkRelease();

		if(!threadCount)
			threadCount = System::ProcessorCount();
		Logger::Debug << "Number of threads: " << threadCount.get() << "\n";

		Stopwatch watch(true);

		std::mutex ioMutex;
		
		std::unique_ptr<rfiStrategy::ForEachMSAction> fomAction(new rfiStrategy::ForEachMSAction());
		if(readMode)
			fomAction->SetIOMode(readMode.get());
		if(readUVW)
			fomAction->SetReadUVW(readUVW.get());
		if(dataColumn)
			fomAction->SetDataColumnName(dataColumn.get());
		if(interval)
			fomAction->SetInterval(interval.get().first, interval.get().second);
		fomAction->SetMaxIntervalSize(maxIntervalSize);
		if(!bands.empty())
			fomAction->Bands() = bands;
		if(!fields.empty())
			fomAction->Fields() = fields;
		if(combineSPWs)
			fomAction->SetCombineSPWs(combineSPWs.get());
		if(bandpass)
			fomAction->SetBandpassFilename(bandpass.get());
		std::stringstream commandLineStr;
		commandLineStr << argv[0];
		for(int i=1;i<argc;++i)
		{
			commandLineStr << " \"" << argv[i] << '\"';
		}
		fomAction->SetCommandLineForHistory(commandLineStr.str());
		if(skipFlagged)
			fomAction->SetSkipIfAlreadyProcessed(skipFlagged.get());
		for(int i=parameterIndex;i<argc;++i)
		{
			Logger::Debug << "Adding '" << argv[i] << "'\n";
			fomAction->Filenames().push_back(argv[i]);
		}
		
		if(strategyFile)
		{
			fomAction->SetLoadOptimizedStrategy(false);
			rfiStrategy::StrategyReader reader;
			std::unique_ptr<rfiStrategy::Strategy> subStrategy;
			try {
				Logger::Debug << "Opening strategy file '" << strategyFile.get() << "'\n";
				subStrategy = reader.CreateStrategyFromFile(strategyFile.get());
				Logger::Debug << "Strategy parsed succesfully.\n";
			} catch(std::exception &e)
			{
				Logger::Error <<
					"ERROR: Reading strategy file \"" << strategyFile.get() << "\" failed! This\n"
					"might be caused by a change in the file format of the strategy file after you\n"
					"created the strategy file.\n"
					"Try recreating the file.\n"
					"\nThe thrown exception was:\n" << e.what() << "\n";
				return RETURN_STRATEGY_PARSE_ERROR;
			}
			if(!rfiStrategy::DefaultStrategy::StrategyContainsAction(*subStrategy, rfiStrategy::ForEachBaselineActionType) &&
				!rfiStrategy::DefaultStrategy::StrategyContainsAction(*subStrategy, rfiStrategy::WriteFlagsActionType))
			{
				rfiStrategy::DefaultStrategy::StrategySetup setup =
					rfiStrategy::DefaultStrategy::DetermineSetup(rfiStrategy::DefaultStrategy::GENERIC_TELESCOPE, 0, 0.0, 0.0, 0.0);
				rfiStrategy::DefaultStrategy::EncapsulateSingleStrategy(*fomAction, std::move(subStrategy), setup);
				Logger::Info << "Modified single-baseline strategy so it will execute strategy on all baselines and write flags.\n";
			}
			else {
				fomAction->Add(std::move(subStrategy));
			}
			if(threadCount)
				rfiStrategy::Strategy::SetThreadCount(*fomAction, threadCount.get());
		}
		else {
			fomAction->SetLoadOptimizedStrategy(true);
			fomAction->Add(std::unique_ptr<rfiStrategy::Strategy>(new rfiStrategy::Strategy())); // This helps the progress reader to determine progress
			if(threadCount)
				fomAction->SetLoadStrategyThreadCount(threadCount.get());
		}
		
		rfiStrategy::Strategy overallStrategy;
		overallStrategy.Add(std::move(fomAction));

		rfiStrategy::ArtifactSet artifacts(&ioMutex);
		artifacts.SetAntennaFlagCountPlot(std::unique_ptr<AntennaFlagCountPlot>(new AntennaFlagCountPlot()));
		artifacts.SetFrequencyFlagCountPlot(std::unique_ptr<FrequencyFlagCountPlot>(new FrequencyFlagCountPlot()));
		artifacts.SetTimeFlagCountPlot(std::unique_ptr<TimeFlagCountPlot>(new TimeFlagCountPlot()));
		artifacts.SetPolarizationStatistics(std::unique_ptr<PolarizationStatistics>(new PolarizationStatistics()));
		artifacts.SetBaselineSelectionInfo(std::unique_ptr<rfiStrategy::BaselineSelector>(new rfiStrategy::BaselineSelector()));
		
		ConsoleProgressHandler progress;

		Logger::Info << "Starting strategy on " << to_simple_string(boost::posix_time::microsec_clock::local_time()) << '\n';
		
		overallStrategy.InitializeAll();
		overallStrategy.StartPerformThread(artifacts, progress);
		std::unique_ptr<rfiStrategy::ArtifactSet> set = overallStrategy.JoinThread();
		overallStrategy.FinishAll();

		set->AntennaFlagCountPlot().Report();
		set->FrequencyFlagCountPlot().Report();
		set->PolarizationStatistics().Report();

		set.reset();

		Logger::Debug << "Time: " << watch.ToString() << "\n";
		
		if(progress.ExceptionOccurred())
			return RETURN_UNHANDLED_EXCEPTION;
		else
			return RETURN_SUCCESS;
	} catch(std::exception& exception)
	{
		std::cerr
			<< "An unhandled exception occured: " << exception.what() << '\n'
			<< "If you think this is a bug, please contact offringa@gmail.com\n";
		return RETURN_UNHANDLED_EXCEPTION;
	}
}
