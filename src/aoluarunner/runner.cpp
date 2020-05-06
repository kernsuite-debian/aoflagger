#include "runner.h"
#include "baselineiterator.h"

#include "../lua/luathreadgroup.h"

#include "../structures/msmetadata.h"

#include "../strategy/imagesets/joinedspwset.h"
#include "../strategy/imagesets/msimageset.h"

#include "../util/logger.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <fstream>

using namespace rfiStrategy;

void Runner::updateOptions(LuaStrategy& lua)
{
	std::vector<std::pair<std::string, std::string>> options = lua.GetOptions();
	for(const std::pair<std::string, std::string>& keyValue : options)
	{
		if(keyValue.first == "baselines")
		{
			if(keyValue.second == "all")
				_options.baselineSelection = rfiStrategy::All;
			else if(keyValue.second == "cross")
				_options.baselineSelection = rfiStrategy::CrossCorrelations;
			else if(keyValue.second == "auto")
				_options.baselineSelection = rfiStrategy::AutoCorrelations;
			else
				throw std::runtime_error("Invalid setting for 'baselines' in function options()");
		}
		else {
			Logger::Warn << "Ignoring unknown key '" + keyValue.first + "' returned by options() function.\n";
		}
	}
}

void Runner::Run()
{
	LuaThreadGroup lua(_options.threadCount.get());
	if(!_options.preamble.empty())
	{
		Logger::Debug << "Running preample...\n";
		lua.RunPreamble(_options.preamble);
	}
	
	if(_options.strategyFile)
	{
		std::string strategyFile = _options.strategyFile.get();
		
		try {
			Logger::Debug << "Opening strategy file '" << strategyFile << "'\n";
			lua.Load(strategyFile.c_str());
			Logger::Debug << "Strategy parsed succesfully.\n";
		} catch(std::exception &e)
		{
			throw std::runtime_error(
				"ERROR: Reading strategy file \"" + strategyFile + "\" failed!\n"
				"\nThe thrown exception was:\n" + e.what() + "\n");
		}
	}
	else {
		// TODO load a default strategy
	}
	
	Logger::Debug << "Loading options from strategy...\n";
	updateOptions(lua.GetThread(0));
	
	Logger::Info << "Starting strategy on " << to_simple_string(boost::posix_time::microsec_clock::local_time()) << '\n';
	
	for(const std::string& filename : _options.filenames)
	{
		processFile(filename, lua);
	}
}

void Runner::processFile(const std::string& filename, LuaThreadGroup& lua)
{
	bool skip = false;
	if(_options.skipFlagged)
	{
		MSMetaData set(filename);
		if(set.HasAOFlaggerHistory())
		{
			skip = true;
			Logger::Info << "Skipping " << filename << ",\n"
				"because the set contains AOFlagger history and -skip-flagged was given.\n";
		}
	}
	
	if(!skip)
	{
		size_t nIntervals = 1;
		size_t intervalIndex = 0;
		size_t resolvedIntStart = 0, resolvedIntEnd = 0;
		bool isMS = false;
		boost::optional<size_t> intervalStart, intervalEnd;
		if(_options.interval)
		{
			intervalStart = _options.interval->first;
			intervalEnd = _options.interval->second;
		}
		while(intervalIndex < nIntervals)
		{
			std::unique_ptr<ImageSet> imageSet(ImageSet::Create(std::vector<std::string>{filename}, _options.readMode.value_or(BaselineIOMode::AutoReadMode)));
			isMS = dynamic_cast<MSImageSet*>(imageSet.get()) != nullptr;
			if(isMS)
			{ 
				MSImageSet* msImageSet = static_cast<MSImageSet*>(imageSet.get());
				msImageSet->SetDataColumnName(_options.dataColumn.get_value_or("DATA"));
				msImageSet->SetReadUVW(_options.readUVW.get_value_or(false));
				// during the first iteration, the nr of intervals hasn't been calculated yet. Do that now.
				if(intervalIndex == 0)
				{
					if(_options.maxIntervalSize)
					{
						msImageSet->SetInterval(intervalStart, intervalEnd);
						const size_t obsTimesSize = msImageSet->MetaData().GetObservationTimesSet().size();
						nIntervals = (obsTimesSize + *_options.maxIntervalSize - 1) / *_options.maxIntervalSize;
						Logger::Info << "Maximum interval size of " << *_options.maxIntervalSize << " timesteps for total of " << obsTimesSize << " timesteps results in " << nIntervals << " intervals.\n";
						if(_options.interval)
						{
							resolvedIntStart = _options.interval->first;
							resolvedIntEnd = _options.interval->second;
						}
						else {
							resolvedIntStart = 0;
							resolvedIntEnd = obsTimesSize + resolvedIntStart;
						}
					}
					else {
						nIntervals = 1;
					}
				}
				if(nIntervals == 1)
					msImageSet->SetInterval(intervalStart, intervalEnd);
				else {
					size_t nTimes = resolvedIntEnd - resolvedIntStart;
					size_t start = resolvedIntStart + intervalIndex*nTimes/nIntervals;
					size_t end = resolvedIntStart + (intervalIndex+1)*nTimes/nIntervals;
					Logger::Info << "Starting flagging of interval " << intervalIndex << ", timesteps " << start << " - " << end << '\n';
					msImageSet->SetInterval(start, end);
				}
				if(_options.combineSPWs)
				{
					msImageSet->Initialize();
					imageSet.release();
					std::unique_ptr<MSImageSet> msImageSetPtr(msImageSet);
					imageSet.reset(new JoinedSPWSet(std::move(msImageSetPtr)));
				}
			}
			imageSet->Initialize();
			
			if(!_options.strategyFile)
			{
				// TODO load strategy
			}
			
			std::mutex ioMutex;
			BaselineIterator blIterator(&ioMutex, _options);
			blIterator.Run(*imageSet, lua);
			
			++intervalIndex;
		}

		if(isMS)
			writeHistory(filename);
	}
}

void Runner::writeHistory(const std::string &filename)
{
	MSMetaData ms(filename);
	Logger::Debug << "Adding strategy to history table of MS...\n";
	try {
		std::string strategyFilename;
		if(_options.strategyFile)
			strategyFilename = *_options.strategyFile;
		else
			/// TODO
			;
		//std::ifstream strategyFile(strategyFilename);
		//std::string content((std::istreambuf_iterator<char>(strategyFile)), (std::istreambuf_iterator<char>()) );
		ms.AddAOFlaggerHistory(strategyFilename, _options.commandLine);
	} catch(std::exception &e)
	{
		Logger::Warn << "Failed to write history to MS: " << e.what() << '\n';
	}
}
