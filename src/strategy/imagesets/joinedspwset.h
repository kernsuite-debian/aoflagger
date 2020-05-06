
#ifndef JOINED_SPW_SET_H
#define JOINED_SPW_SET_H

#include "imageset.h"
#include "msimageset.h"

#include "../../structures/msmetadata.h"

#include <vector>
#include <list>
#include <map>

namespace rfiStrategy {
	
	using Sequence = MSMetaData::Sequence;
	
	class JoinedSPWSetIndex : public ImageSetIndex {
		public:
			friend class JoinedSPWSet;
			
			explicit JoinedSPWSetIndex(class JoinedSPWSet &set);
			virtual ~JoinedSPWSetIndex() override { }
			virtual void Previous() override final;
			virtual void Next() override final;
			virtual std::string Description() const override final;
			virtual bool IsValid() const override final { return _isValid; }
			virtual std::unique_ptr<ImageSetIndex> Clone() const override final
			{ return std::unique_ptr<JoinedSPWSetIndex>(new JoinedSPWSetIndex(*this) ); }
			
		private:
			JoinedSPWSetIndex(const JoinedSPWSetIndex&) = default;
			JoinedSPWSetIndex& operator=(const JoinedSPWSetIndex&) = delete;
			virtual void reattach() final override;
			
			std::map<Sequence, std::vector<std::pair<size_t, size_t>>>::const_iterator _iterator;
			bool _isValid;
	};
	
	class JoinedSPWSet : public IndexableSet
	{
	public:
		/**
		 * @param msImageSet An already initialized image set of which ownership is transferred to
		 * this class.
		 */
		explicit JoinedSPWSet(std::unique_ptr<MSImageSet> msImageSet) :
			_msImageSet(std::move(msImageSet))
		{
			const std::vector<MSMetaData::Sequence>& sequences = _msImageSet->Sequences();
			size_t nBands = _msImageSet->BandCount();
			_nChannels.resize(nBands);
			for(size_t b=0; b!=nBands; ++b)
				_nChannels[b] = _msImageSet->GetBandInfo(b).channels.size();
			for(size_t sequenceIndex = 0; sequenceIndex!=sequences.size(); ++sequenceIndex)
			{
				const MSMetaData::Sequence& s = sequences[sequenceIndex];
				Sequence js(s.antenna1, s.antenna2, 0, s.sequenceId, s.fieldId);
				// TODO Central frequency instead of spw id is a better idea
				_joinedSequences[js].emplace_back(s.spw, sequenceIndex);
			}
			for(auto& js : _joinedSequences)
				std::sort(js.second.begin(), js.second.end());
		}
		
		JoinedSPWSet(const JoinedSPWSet& source) = delete;
		JoinedSPWSet& operator=(const JoinedSPWSet& source) = delete;
		
		virtual ~JoinedSPWSet() override { };
		virtual std::unique_ptr<ImageSet> Clone() override final
		{
			std::unique_ptr<JoinedSPWSet> newSet(new JoinedSPWSet());
			newSet->_msImageSet.reset(new MSImageSet(*_msImageSet));
			newSet->_joinedSequences = _joinedSequences;
			newSet->_nChannels = _nChannels;
			return std::move(newSet);
		}

		virtual std::unique_ptr<ImageSetIndex> StartIndex() override final
		{
			return std::unique_ptr<JoinedSPWSetIndex>(new JoinedSPWSetIndex(*this));
		}
		
		virtual void Initialize() override final { }
		virtual std::string Name() override final
		{ return _msImageSet->Name() + " (SPWs joined)"; }
		virtual std::string File() override final
		{ return _msImageSet->File(); }
		
		virtual void AddReadRequest(const ImageSetIndex &index) override final
		{
			const std::vector<std::pair<size_t /*spw*/, size_t /*seq*/>>& indexInformation =
				static_cast<const JoinedSPWSetIndex&>(index)._iterator->second;
			
			for(const std::pair<size_t, size_t>& spwAndSeq : indexInformation)
			{
				MSImageSetIndex msIndex(*_msImageSet, spwAndSeq.second);
				_msImageSet->AddReadRequest(msIndex);
			}
			_requests.push_back(static_cast<const JoinedSPWSetIndex&>(index)._iterator);
		}
		
		virtual void PerformReadRequests() override final
		{
			_msImageSet->PerformReadRequests();
			for(auto& request : _requests)
			{
				const std::vector<std::pair<size_t /*spw*/, size_t /*seq*/>>& indexInformation =
					request->second;
				std::vector<std::unique_ptr<BaselineData>> data;
				size_t totalHeight = 0;
				for(size_t i=0; i!=indexInformation.size(); ++i)
				{
					data.emplace_back(_msImageSet->GetNextRequested());
					totalHeight += data.back()->Data().ImageHeight();
				}
				
				// Combine the images
				TimeFrequencyData tfData(data[0]->Data());
				size_t width = tfData.ImageWidth();
				for(size_t imgIndex=0; imgIndex!=tfData.ImageCount(); ++imgIndex)
				{
					size_t chIndex = 0;
					Image2DPtr img = Image2D::CreateUnsetImagePtr(width, totalHeight);
					for(size_t i=0; i!=data.size(); ++i)
					{
						Image2DCPtr src = data[i]->Data().GetImage(imgIndex);
						for(size_t y=0; y!=src->Height(); ++y)
						{
							num_t* destPtr = img->ValuePtr(0, y+chIndex);
							const num_t* srcPtr = src->ValuePtr(0, y);
							for(size_t x=0; x!=src->Width(); ++x)
								destPtr[x] = srcPtr[x];
						}
						chIndex += data[i]->Data().ImageHeight();
					}
					tfData.SetImage(imgIndex, img);
				}
				
				// Combine the masks
				for(size_t maskIndex=0; maskIndex!=tfData.MaskCount(); ++maskIndex)
				{
					size_t chIndex = 0;
					Mask2DPtr mask = Mask2D::CreateUnsetMaskPtr(width, totalHeight);
					for(size_t i=0; i!=data.size(); ++i)
					{
						Mask2DCPtr src = data[i]->Data().GetMask(maskIndex);
						for(size_t y=0; y!=src->Height(); ++y)
						{
							bool* destPtr = mask->ValuePtr(0, y+chIndex);
							const bool* srcPtr = src->ValuePtr(0, y);
							for(size_t x=0; x!=src->Width(); ++x)
								destPtr[x] = srcPtr[x];
						}
						chIndex += data[i]->Data().ImageHeight();
					}
					tfData.SetMask(maskIndex, mask);
				}
				
				// Combine the metadata
				TimeFrequencyMetaDataPtr metaData(new TimeFrequencyMetaData(*data[0]->MetaData()));
				BandInfo band = metaData->Band();
				size_t chIndex = band.channels.size();
				band.channels.resize(totalHeight);
				for(size_t i=1; i!=data.size(); ++i)
				{
					const BandInfo& srcBand = data[i]->MetaData()->Band();
					for(size_t ch=0; ch!=srcBand.channels.size(); ++ch)
						band.channels[ch + chIndex] = srcBand.channels[ch];
					chIndex += srcBand.channels.size();
				}
				
				metaData->SetBand(band);
				
				std::unique_ptr<ImageSetIndex> index = Index(request->first.antenna1, request->first.antenna2, request->first.spw, request->first.sequenceId);
				BaselineData combinedData(tfData, metaData, *index);
				_baselineData.push_back(combinedData);
			}
			_requests.clear();
		}
		
		virtual std::unique_ptr<BaselineData> GetNextRequested() override final
		{
			std::unique_ptr<BaselineData> data(new BaselineData(_baselineData.front()));
			_baselineData.pop_front();
			return std::move(data);
		}
		
		virtual void AddWriteFlagsTask(const ImageSetIndex &index, std::vector<Mask2DCPtr> &flags) override final
		{
			const std::vector<std::pair<size_t /*spw*/, size_t /*seq*/>>& indexInformation =
				static_cast<const JoinedSPWSetIndex&>(index)._iterator->second;
			size_t width = flags.front()->Width();
			size_t chIndex = 0;
			for(size_t spw=0; spw!=indexInformation.size(); ++spw)
			{
				const std::pair<size_t, size_t>& spwAndSeq = indexInformation[spw];
				std::vector<Mask2DCPtr> spwFlags(flags.size());
				for(size_t m=0; m!=flags.size(); ++m)
				{
					Mask2DPtr spwMask = Mask2D::CreateUnsetMaskPtr(width, _nChannels[spwAndSeq.first]);
					for(size_t y=0; y!=_nChannels[spwAndSeq.first]; ++y)
					{
						const bool *srcPtr = flags[m]->ValuePtr(0, y + chIndex);
						bool *destPtr = spwMask->ValuePtr(0, y);
						for(size_t x=0; x!=flags[m]->Width(); ++x)
							destPtr[x] = srcPtr[x];
					}
					spwFlags[m] = std::move(spwMask);
				}
				chIndex += _nChannels[spw];
				
				MSImageSetIndex msIndex(*_msImageSet, spwAndSeq.second);
				_msImageSet->AddWriteFlagsTask(msIndex, spwFlags);
			}
		}
		virtual void PerformWriteFlagsTask() override final
		{
			_msImageSet->PerformWriteFlagsTask();
		}
		
		virtual BaselineReaderPtr Reader() const override final
		{
			return _msImageSet->Reader();
		}
		
		const std::map<Sequence, std::vector<std::pair<size_t, size_t>>>& JoinedSequences() const { return _joinedSequences; }
		
		MSImageSet& msImageSet() { return *_msImageSet; }
		
		virtual size_t AntennaCount() const override final
		{
			return _msImageSet->AntennaCount();
		}
		
		virtual size_t GetAntenna1(const ImageSetIndex &index) override final
		{
			return static_cast<const JoinedSPWSetIndex&>(index)._iterator->first.antenna1;
		}
		
		virtual size_t GetAntenna2(const ImageSetIndex &index) override final
		{
			return static_cast<const JoinedSPWSetIndex&>(index)._iterator->first.antenna2;
		}
		
		virtual size_t GetBand(const ImageSetIndex &index) override final
		{
			return 0;
		}
		
		virtual size_t GetField(const ImageSetIndex &index) override final
		{
			return static_cast<const JoinedSPWSetIndex&>(index)._iterator->first.fieldId;
		}
		
		virtual size_t GetSequenceId(const ImageSetIndex &index) override final
		{
			return static_cast<const JoinedSPWSetIndex&>(index)._iterator->first.sequenceId;
		}
		
		virtual AntennaInfo GetAntennaInfo(unsigned antennaIndex) override final
		{
			return _msImageSet->GetAntennaInfo(antennaIndex);
		}
		
		virtual size_t BandCount() const override final
		{
			return 1;
		}
		
		virtual BandInfo GetBandInfo(unsigned bandIndex) override final
		{
			BandInfo band = _msImageSet->GetBandInfo(0);
			size_t chIndex = band.channels.size();
			for(size_t i=1; i!=_msImageSet->BandCount(); ++i)
			{
				const BandInfo srcBand = _msImageSet->GetBandInfo(i);
				band.channels.resize(chIndex + srcBand.channels.size());
				for(size_t ch=0; ch!=srcBand.channels.size(); ++ch)
					band.channels[ch + chIndex] = srcBand.channels[ch];
				chIndex += srcBand.channels.size();
			}
			return band;
		}
		
		virtual size_t SequenceCount() const override final
		{
			return _msImageSet->SequenceCount();
		}
		
		virtual std::unique_ptr<ImageSetIndex> Index(size_t antenna1, size_t antenna2, size_t bandIndex, size_t sequenceId) override final
		{
			for(auto i=_joinedSequences.begin(); i != _joinedSequences.end() ; ++i)
			{
				bool antennaMatch = (i->first.antenna1 == antenna1 && i->first.antenna2 == antenna2) || (i->first.antenna1 == antenna2 && i->first.antenna2 == antenna1);
				if(antennaMatch && i->first.sequenceId == sequenceId)
				{
					std::unique_ptr<JoinedSPWSetIndex> index(new JoinedSPWSetIndex(*this));
					index->_iterator = i;
					return std::move(index);
				}
			}
			std::stringstream str;
			str << "Baseline not found: "
				<< "antenna1=" << antenna1 << ", "
				<< "antenna2=" << antenna2 << ", "
				<< "sequenceId=" << sequenceId;
			throw BadUsageException(str.str());
		}
		
		virtual FieldInfo GetFieldInfo(unsigned fieldIndex) override final
		{
			return _msImageSet->GetFieldInfo(fieldIndex);
		}
	private:
		JoinedSPWSet() = default;
		
		std::unique_ptr<MSImageSet> _msImageSet;
		std::map<Sequence, std::vector<std::pair<size_t, size_t>>> _joinedSequences;
		std::vector<std::map<Sequence, std::vector<std::pair<size_t, size_t>>>::const_iterator> _requests;
		std::list<BaselineData> _baselineData;
		std::vector<size_t> _nChannels;
	};
	
	JoinedSPWSetIndex::JoinedSPWSetIndex(class JoinedSPWSet &set) :
		ImageSetIndex(set),
		_iterator(static_cast<JoinedSPWSet&>(imageSet()).JoinedSequences().begin()),
		_isValid(true)
	{
	}
	
	void JoinedSPWSetIndex::Next()
	{
		++_iterator;
		if(_iterator == static_cast<JoinedSPWSet&>(imageSet()).JoinedSequences().end())
		{
			_iterator = static_cast<JoinedSPWSet&>(imageSet()).JoinedSequences().begin();
			_isValid = false;
		}
	}

	void JoinedSPWSetIndex::Previous()
	{
		if(_iterator == static_cast<JoinedSPWSet&>(imageSet()).JoinedSequences().begin())
		{
			_iterator = static_cast<JoinedSPWSet&>(imageSet()).JoinedSequences().end();
			_isValid = false;
		}
		--_iterator;
	}

	void JoinedSPWSetIndex::reattach()
	{
		_iterator = static_cast<JoinedSPWSet&>(imageSet()).JoinedSequences().find(_iterator->first);
	}

	std::string JoinedSPWSetIndex::Description() const
	{
		MSImageSet& msImageSet = static_cast<JoinedSPWSet&>(imageSet()).msImageSet();
		
		const Sequence &sequence = _iterator->first;
		size_t
			antenna1 = sequence.antenna1,
			antenna2 = sequence.antenna2,
			sequenceId = sequence.sequenceId;
		AntennaInfo info1 = msImageSet.GetAntennaInfo(antenna1);
		AntennaInfo info2 = msImageSet.GetAntennaInfo(antenna2);
		std::stringstream sstream;
		sstream
			<< info1.station << ' ' << info1.name << " x " << info2.station << ' ' << info2.name
			<< " (joined spws)";
		if(static_cast<class MSImageSet&>(imageSet()).SequenceCount() > 1)
		{
			sstream
				<< ", seq " << sequenceId;
		}
		return sstream.str();
	}
}

#endif
