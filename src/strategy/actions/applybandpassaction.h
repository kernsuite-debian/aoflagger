#ifndef RFISTRATEGY_APPLY_BANDPASS_ACTION_H
#define RFISTRATEGY_APPLY_BANDPASS_ACTION_H

#include <fstream>
#include <map>
#include <mutex>

#include "action.h"

#include "../../util/logger.h"

#include "../control/artifactset.h"

namespace rfiStrategy {

class ApplyBandpassAction : public Action {
public:
	ApplyBandpassAction()
	{ }
	
	virtual std::string Description() final override
	{
		return "Apply bandpass";
	}
	
	virtual void Perform(ArtifactSet& artifacts, ProgressListener&) final override
	{
		if(!artifacts.HasMetaData() || !artifacts.MetaData()->HasAntenna1() || !artifacts.MetaData()->HasAntenna2())
			throw std::runtime_error("No meta data available for bandpass correction");
		
		apply(artifacts.ContaminatedData(), artifacts.MetaData()->Antenna1().name, artifacts.MetaData()->Antenna2().name);
	}

	virtual ActionType Type() const final override { return ApplyBandpassType; }

	const std::string& Filename() const { return _filename; }
	void SetFilename(const std::string& filename) { _filename = filename; _file.reset(); }
	
private:
	struct BandpassIndex
	{
		std::string antenna;
		char polarization;
		size_t channel;
		
		bool operator<(const BandpassIndex& rhs) const {
			if(channel < rhs.channel)
				return true;
			else if(channel == rhs.channel)
			{
				if(polarization < rhs.polarization)
					return true;
				else if(polarization == rhs.polarization && antenna < rhs.antenna)
					return true;
			}
			return false;
		}
	};
	
	class BandpassFile
	{
	public:
		BandpassFile(const std::string& filename)
		{
			std::ifstream file(filename);
			if(!file)
				throw std::runtime_error("Can not find bandpasss file: '" + filename + '\'');
			std::string antenna, pol;
			size_t channel;
			double value;
			while(file)
			{
				file >> antenna >> pol >> channel >> value;
				if(file.good())
				{
					char polChar = pol[0];
					_values.emplace(BandpassIndex{antenna, polChar, channel}, value);
					//Logger::Info << antenna << " , " << polChar << " , " << channel << '\n';
				}
			}
			Logger::Info << "Read " << _values.size() << " passband values from file " << filename << ".\n";
		}
		
		double GetValue(const std::string& antenna, char polarization, size_t channel) const
		{
			auto iter = _values.find(BandpassIndex{antenna, polarization, channel});
			if(iter == _values.end())
			{
				throw std::runtime_error("Passband file is missing values for " + antenna + " pol " + polarization + " ch " + std::to_string(channel));
			}
			return iter->second;
		}
		
		/** antenna, polarization, */
		std::map<BandpassIndex, double> _values;
	};
	
	BandpassFile& getFile() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if(_file == nullptr)
		{
			_file.reset(new BandpassFile(_filename));
		}
		return *_file.get();
	}
	
	void apply(TimeFrequencyData& data, const std::string& antenna1, const std::string& antenna2) const
	{
		BandpassFile& file = getFile();
		for(size_t i=0; i!=data.PolarizationCount(); ++i)
		{
			TimeFrequencyData polData = data.MakeFromPolarizationIndex(i);
			char pol1, pol2;
			switch(polData.GetPolarization(0))
			{
				case Polarization::XX: pol1 = 'X'; pol2 = 'X'; break;
				case Polarization::XY: pol1 = 'X'; pol2 = 'Y'; break;
				case Polarization::YX: pol1 = 'Y'; pol2 = 'X'; break;
				case Polarization::YY: pol1 = 'Y'; pol2 = 'Y'; break;
				case Polarization::LL: pol1 = 'L'; pol2 = 'L'; break;
				case Polarization::LR: pol1 = 'L'; pol2 = 'R'; break;
				case Polarization::RL: pol1 = 'R'; pol2 = 'L'; break;
				case Polarization::RR: pol1 = 'R'; pol2 = 'R'; break;
				default: throw std::runtime_error("Can't apply a bandpass for polarization " + Polarization::TypeToShortString(polData.GetPolarization(0)));
			}
			
			for(size_t imageIndex=0; imageIndex!=polData.ImageCount(); ++imageIndex)
			{
				Image2DCPtr uncorrected = polData.GetImage(imageIndex);
				Image2DPtr newImage = apply(uncorrected, file, antenna1, antenna2, pol1, pol2);
				polData.SetImage(imageIndex, std::move(newImage));
			}
			data.SetPolarizationData(i, std::move(polData));
		}
	}
	
	Image2DPtr apply(Image2DCPtr& uncorrected, BandpassFile& file, const std::string& antenna1, const std::string& antenna2, char pol1, char pol2) const
	{
		Image2DPtr corrected = Image2D::MakePtr(uncorrected->Width(), uncorrected->Height());
		for(size_t ch=0; ch!=uncorrected->Height(); ++ch)
		{
			float val = 1.0/(file.GetValue(antenna1, pol1, ch) * file.GetValue(antenna2, pol2, ch));
			const num_t* ptrUncor = uncorrected->ValuePtr(0, ch);
			num_t* ptrCor = corrected->ValuePtr(0, ch);
			for(size_t x=0; x!=uncorrected->Width(); ++x)
			{
				ptrCor[x] = ptrUncor[x] * val;
			}
		}
		return corrected;
	}
	
	std::string _filename;
	mutable std::mutex _mutex;
	mutable std::unique_ptr<BandpassFile> _file;
};
}

#endif

