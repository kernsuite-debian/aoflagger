#ifndef INDEXABLE_SET_H
#define INDEXABLE_SET_H

#include "imageset.h"

#include "../../msio/baselinereader.h"

#include <memory>
#include <string>

namespace rfiStrategy
{
	class IndexableSet : public ImageSet
	{
	public:
		virtual BaselineReaderPtr Reader() const = 0;
		virtual size_t GetAntenna1(const ImageSetIndex &index) = 0;
		virtual size_t GetAntenna2(const ImageSetIndex &index) = 0;
		virtual size_t GetBand(const ImageSetIndex &index) = 0;
		virtual size_t GetField(const ImageSetIndex &index) = 0;
		virtual size_t GetSequenceId(const ImageSetIndex &index) = 0;
		virtual size_t AntennaCount() const = 0;
		virtual AntennaInfo GetAntennaInfo(unsigned antennaIndex) = 0;
		virtual size_t BandCount() const = 0;
		virtual BandInfo GetBandInfo(unsigned bandIndex) = 0;
		virtual size_t SequenceCount() const = 0;
		virtual std::unique_ptr<ImageSetIndex> Index(size_t antenna1, size_t antenna2, size_t bandIndex, size_t sequenceId) = 0;
		virtual FieldInfo GetFieldInfo(unsigned fieldIndex) = 0;

		virtual std::string TelescopeName() final override;
	};
}
	
inline std::string rfiStrategy::IndexableSet::TelescopeName()
{
	return Reader()->Set().TelescopeName();
}

#endif

