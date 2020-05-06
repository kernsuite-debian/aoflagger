#include "rfibaselineset.h"

#include "../../msio/singlebaselinefile.h"

#include <fstream>
#include <sstream>

namespace rfiStrategy {

RFIBaselineSet::RFIBaselineSet(const std::string& path) :
	SingleImageSet(), _path(path)
{
	SingleBaselineFile file;
	std::ifstream str(_path);
	file.Read(str);
	_data = std::move(file.data);
	_metaData = std::move(file.metaData);
	_telescopeName = std::move(file.telescopeName);
}

std::unique_ptr<BaselineData> RFIBaselineSet::Read()
{
	return std::unique_ptr<BaselineData>(new BaselineData(
		_data,
		TimeFrequencyMetaDataPtr(new TimeFrequencyMetaData(_metaData))
	));
}

std::string RFIBaselineSet::BaselineDescription()
{
	if(_metaData.HasAntenna1() && _metaData.HasAntenna2())
	{
		std::ostringstream sstream;
		sstream
			<< _metaData.Antenna1().station << ' ' << _metaData.Antenna1().name 
			<< " x "
			<< _metaData.Antenna2().station << ' ' << _metaData.Antenna2().name;
		return sstream.str();
	}
	else return File();
}

void RFIBaselineSet::Write(const std::vector<Mask2DCPtr>& masks)
{
	SingleBaselineFile file;
	{
		std::ifstream str(_path);
		file.Read(str);
	}
	if(file.data.MaskCount() != masks.size())
		throw std::runtime_error("Number of masks in flag writing action don't match rfibl file");
	for(size_t i=0; i!=masks.size(); ++i)
		file.data.SetMask(i, masks[i]);
	std::ofstream outstr(_path);
	file.Write(outstr);
}

}
