#ifndef FILTERBANKSET_H
#define FILTERBANKSET_H

#include <string>
#include <deque>

#include "imageset.h"

#include "../../util/logger.h"

namespace rfiStrategy {

	class FilterBankSetIndex : public ImageSetIndex {
		public:
			friend class FilterBankSet;
			
			explicit FilterBankSetIndex(class rfiStrategy::ImageSet &set) :
				ImageSetIndex(set), _intervalIndex(0), _isValid(true)
			{ }
			
			virtual void Previous() final override;
			virtual void Next() final override;
			virtual std::string Description() const final override;
			virtual bool IsValid() const final override { return _isValid; }
			virtual std::unique_ptr<ImageSetIndex> Clone() const final override
			{
				std::unique_ptr<FilterBankSetIndex> index( new FilterBankSetIndex(imageSet()) );
				index->_intervalIndex = _intervalIndex;
				index->_isValid = _isValid;
				return std::move(index);
			}
		private:
			size_t _intervalIndex;
			bool _isValid;
	};
	
	class FilterBankSet : public ImageSet {
		public:
			explicit FilterBankSet(const std::string &location);
			
			~FilterBankSet()
			{ }

			virtual std::unique_ptr<ImageSet> Clone() final override
			{
				std::unique_ptr<FilterBankSet> set(new FilterBankSet(*this));
				set->_requests.clear();
				return std::move(set);
			}
	
			virtual std::string Name() final override { return _location; }
			
			virtual std::string File() final override { return _location; }
			
			virtual std::string TelescopeName() final override;
			
			virtual void AddReadRequest(const ImageSetIndex &index) final override;
			
			virtual void PerformReadRequests() final override;
			
			virtual std::unique_ptr<BaselineData> GetNextRequested() final override;

			virtual void AddWriteFlagsTask(const ImageSetIndex &index, std::vector<Mask2DCPtr> &flags) final override;
			
			virtual void Initialize() final override;
	
			virtual std::unique_ptr<ImageSetIndex> StartIndex() final override
			{
				return std::unique_ptr<ImageSetIndex>(new FilterBankSetIndex(*this));
			}

			virtual void PerformWriteDataTask(const ImageSetIndex &index, std::vector<Image2DCPtr> realImages, std::vector<Image2DCPtr> imaginaryImages) final override;
			
			double CentreFrequency() const
			{
				return (_fch1 + (_foff*_channelCount*0.5))*1e6;
			}
			double ChannelWidth() const
			{
				return std::fabs(_foff)*1e6;
			}
			double TimeResolution() const
			{
				return _timeOfSample;
			}
		private:
			friend class FilterBankSetIndex;
			std::string _location;
			
			double _timeOfSample, _timeStart, _fch1, _foff;
			size_t _channelCount, _ifCount, _bitCount, _sampleCount;
			size_t _nBeams, _iBeam;
			int _machineId, _telescopeId;
			size_t _intervalCount;
			std::streampos _headerEnd;
			
			std::deque<BaselineData*> _requests;
			
			static int32_t readInt(std::istream& str)
			{
				int32_t val;
				str.read(reinterpret_cast<char*>(&val), sizeof(int32_t));
				return val;
			}
			
			static double readDouble(std::istream& str)
			{
				double val;
				str.read(reinterpret_cast<char*>(&val), sizeof(double));
				return val;
			}
			
			static std::string readString(std::istream& str)
			{
				int32_t length = readInt(str);
				if(length <= 0 || length >= 80)
					return std::string();
				std::string data(length, 0);
				str.read(&data[0], length);
				return std::string(&data[0]);
			}
	};

}
	
#endif
