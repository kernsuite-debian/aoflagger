#ifndef PYTHON_SCRIPTDATA_H
#define PYTHON_SCRIPTDATA_H

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

#include "../strategy/algorithms/bandpassfile.h"

#include "../structures/timefrequencydata.h"

class ScriptData
{
public:
	ScriptData() :
		_bandpassFile(nullptr),
		_canVisualize(false)
	{ }
	
	std::unique_ptr<BandpassFile>& GetBandpassFile()
	{
		return _bandpassFile;
	}
	
	std::mutex& BandpassMutex() { return _bandpassMutex; }
		
	void AddVisualization(TimeFrequencyData& data, const std::string& label, size_t sortingIndex)
	{
		if(_canVisualize)
		{
			if(data.PolarizationCount() == 1)
			{
				PolarizationEnum p = data.GetPolarization(0);
				for(auto& v : _visualizationData)
				{
					if(std::get<0>(v) == label)
					{
						if(std::get<1>(v).HasPolarization(p))
						{
							// Can't merge, continue search and
							// if not found add like normal
						}
						else {
							// Merge
							std::get<1>(v) = TimeFrequencyData::MakeFromPolarizationCombination(std::get<1>(v), data);
							return;
						}
					}
				}
				// Label not found, add
				_visualizationData.emplace_back(label, data, sortingIndex);
			}
			else {
				_visualizationData.emplace_back(label, data, sortingIndex);
			}
		}
	}
	
	size_t VisualizationCount() const { return _visualizationData.size(); }
	std::tuple<std::string, TimeFrequencyData, size_t> GetVisualization(size_t index) const
	{ return _visualizationData[index]; }
	
	void SetCanVisualize(bool canVisualize)
	{ _canVisualize = canVisualize; }
	
	void SortVisualizations()
	{
			// Sort the visualizations on their sorting index
		std::sort(_visualizationData.begin(), _visualizationData.end(),
						[](const std::tuple<std::string, TimeFrequencyData, size_t>& a,
								const std::tuple<std::string, TimeFrequencyData, size_t>& b)
		{ return std::get<2>(a) < std::get<2>(b); });
	}
	
private:
	std::unique_ptr<BandpassFile> _bandpassFile;
	std::mutex _bandpassMutex;
	bool _canVisualize;
	std::vector<std::tuple<std::string, TimeFrequencyData, size_t>> _visualizationData;
};

#endif
