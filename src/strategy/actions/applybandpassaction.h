#ifndef RFISTRATEGY_APPLY_BANDPASS_ACTION_H
#define RFISTRATEGY_APPLY_BANDPASS_ACTION_H

#include <fstream>
#include <map>
#include <mutex>

#include "action.h"

#include "../../util/logger.h"

#include "../algorithms/applybandpass.h"
#include "../algorithms/bandpassfile.h"

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
		
		ApplyBandpass::Apply(artifacts.ContaminatedData(), getFile(), artifacts.MetaData()->Antenna1().name, artifacts.MetaData()->Antenna2().name);
	}

	virtual ActionType Type() const final override { return ApplyBandpassType; }

	const std::string& Filename() const { return _filename; }
	void SetFilename(const std::string& filename) { _filename = filename; _file.reset(); }
	
private:
	BandpassFile& getFile() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if(_file == nullptr)
		{
			_file.reset(new BandpassFile(_filename));
		}
		return *_file.get();
	}
	
	std::string _filename;
	mutable std::mutex _mutex;
	mutable std::unique_ptr<BandpassFile> _file;
};
}

#endif

