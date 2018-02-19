#ifndef POLARIZATIONSTATISTICS_H
#define POLARIZATIONSTATISTICS_H

#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>

#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include "../../util/logger.h"

class PolarizationStatistics {
	public:
		PolarizationStatistics() { }
		~PolarizationStatistics() { }

		void Add(const class TimeFrequencyData &data)
		{
			unsigned polarizationCount = data.PolarizationCount();
			if(_flaggedCounts.size() == 0)
			{
				_polarizations = data.Polarizations();
				for(unsigned i=0;i<polarizationCount;++i)
				{
					_flaggedCounts.push_back(0);
					_totalCounts.push_back(0);
					_names.push_back(Polarization::TypeToFullString(_polarizations[i]));
				}
			} else if(_polarizations != data.Polarizations())
			{
				throw std::runtime_error("Adding differently polarized data to statistics");
			}
			for(unsigned i=0;i<polarizationCount;++i)
			{
				Mask2DCPtr mask = data.MakeFromPolarizationIndex(i).GetSingleMask();
				_flaggedCounts[i] += mask->GetCount<true>();
				_totalCounts[i] += mask->Width() * mask->Height();
			}
		}
		bool HasData() { return !_flaggedCounts.empty(); }
		void Report()
		{
			if(HasData())
			{
				Logger::Info
					<< "Polarization statistics: ";
				for(unsigned i=0;i<_flaggedCounts.size();++i)
				{
					numl_t percentage = (numl_t) _flaggedCounts[i] * 100.0 / (numl_t) _totalCounts[i];
					if(i!=0)
						Logger::Info << ", ";
					Logger::Info
						<< _names[i] << ": " << formatPercentage(percentage) << '%';
				}
				Logger::Info << '\n';
			} else {
				Logger::Info
					<< "No polarization statistics were collected.\n";
			}
		}
	private:
		std::string formatPercentage(numl_t percentage)
		{
			std::ostringstream s;
			if(percentage >= 1.0)
				s << round(percentage*10.0)/10.0;
			else if(percentage >= 0.1)
				s << round(percentage*100.0)/100.0;
			else
				s << round(percentage*1000.0)/1000.0;
			return s.str();
		}

		std::vector<long unsigned> _flaggedCounts, _totalCounts;
		std::vector<std::string> _names;
		std::vector<PolarizationEnum> _polarizations;
};

#endif
