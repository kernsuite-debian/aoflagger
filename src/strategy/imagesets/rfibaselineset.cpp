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

}
