#ifndef STATISTICS_COLLECTION_H
#define STATISTICS_COLLECTION_H

#include <stdint.h>

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

#include "../util/serializable.h"

#include "baselinestatisticsmap.h"
#include "defaultstatistics.h"
#include "qualitytablesformatter.h"
#include "statisticalvalue.h"

#include <boost/concept_check.hpp>

class StatisticsCollection : public Serializable
{
	private:
		typedef std::map<double, DefaultStatistics> DoubleStatMap;
	public:
		StatisticsCollection() : _polarizationCount(0), _emptyBaselineStatisticsMap(0)
		{
		}
		
		explicit StatisticsCollection(unsigned polarizationCount) : _polarizationCount(polarizationCount), _emptyBaselineStatisticsMap(polarizationCount)
		{
		}
		
		StatisticsCollection(const StatisticsCollection &source) :
			_timeStatistics(source._timeStatistics),
			_frequencyStatistics(source._frequencyStatistics),
			_baselineStatistics(source._baselineStatistics),
			_polarizationCount(source._polarizationCount),
			_emptyBaselineStatisticsMap(source._polarizationCount)
		{
		}
		
		StatisticsCollection & operator=(const StatisticsCollection &source)
		{
			_timeStatistics = source._timeStatistics;
			_frequencyStatistics = source._frequencyStatistics;
			_baselineStatistics = source._baselineStatistics;
			_polarizationCount = source._polarizationCount;
			_emptyBaselineStatisticsMap = source._emptyBaselineStatisticsMap;
			return *this;
		}

		void Clear()
		{
			_timeStatistics.clear();
			_frequencyStatistics.clear();
			_baselineStatistics.clear();
		}
		
		void InitializeBand(unsigned band, const double *frequencies, unsigned channelCount)
		{
			std::vector<DefaultStatistics *> pointers;
			for(unsigned i=0;i<channelCount;++i)
			{
				pointers.push_back(&getFrequencyStatistic(frequencies[i]));
			}
			_bands.insert(std::pair<unsigned, std::vector<DefaultStatistics *> >(band, pointers));
			double centralFrequency = (frequencies[0] + frequencies[channelCount-1]) / 2.0;
			_centralFrequencies.insert(std::pair<unsigned, double>(band, centralFrequency));
			
			std::vector<double> freqVect(frequencies, frequencies+channelCount);
			_bandFrequencies.insert(std::pair<double, std::vector<double> >(band, freqVect));
		}
		
		void Add(unsigned antenna1, unsigned antenna2, double time, unsigned band, int polarization, const float* reals, const float* imags, const bool* isRFI, const bool* origFlags, unsigned nsamples, unsigned step, unsigned stepRFI, unsigned stepFlags);
		
		void Add(unsigned antenna1, unsigned antenna2, double time, unsigned band, int polarization, const std::vector<std::complex<float> >& samples, const bool* isRFI)
		{
			const float *dataPtr = reinterpret_cast<const float*>(&(samples[0]));
			
			bool origFlag = false;
			Add(antenna1, antenna2, time, band, polarization,
					dataPtr, dataPtr+1,   // real and imag parts
					isRFI, &origFlag, samples.size(), 2, 1, 0);
		}
		
		void AddToTimeFrequency(unsigned antenna1, unsigned antenna2, double time, unsigned band, int polarization, const float* reals, const float* imags, const bool* isRFI, const bool* origFlags, unsigned nsamples, unsigned step, unsigned stepRFI, unsigned stepFlags);
		
		void AddImage(unsigned antenna1, unsigned antenna2, const double* times, unsigned band, int polarization, const Image2DCPtr& realImage, const Image2DCPtr& imagImage, const Mask2DCPtr& rfiMask, const Mask2DCPtr& correlatorMask);
		
		void Save(QualityTablesFormatter &qualityData) const
		{
			saveTime(qualityData);
			saveFrequency(qualityData);
			saveBaseline(qualityData);
		}
		
		void Load(QualityTablesFormatter &qualityData)
		{
			SetPolarizationCount(qualityData.GetPolarizationCount());
			loadTime<false>(qualityData);
			loadFrequency<false>(qualityData);
			loadBaseline<false>(qualityData);
		}
		
		void LoadTimeStatisticsOnly(QualityTablesFormatter &qualityData)
		{
			loadTime<false>(qualityData);
		}
		
		void Add(QualityTablesFormatter &qualityData)
		{
			loadTime<true>(qualityData);
			loadFrequency<true>(qualityData);
			loadBaseline<true>(qualityData);
		}
		
		void Add(const StatisticsCollection &collection)
		{
			addTime(collection);
			addFrequency(collection);
			addBaseline(collection);
		}
		
		void GetGlobalTimeStatistics(DefaultStatistics &statistics)
		{
			statistics = getGlobalStatistics(_timeStatistics);
		}
		
		void GetGlobalFrequencyStatistics(DefaultStatistics &statistics)
		{
			statistics = getGlobalStatistics(_frequencyStatistics);
		}
		
		void GetGlobalAutoBaselineStatistics(DefaultStatistics &statistics) const
		{
			statistics = getGlobalBaselineStatistics<true>();
		}
		
		void GetGlobalCrossBaselineStatistics(DefaultStatistics &statistics) const
		{
			statistics = getGlobalBaselineStatistics<false>();
		}
		
		void GetFrequencyRangeStatistics(DefaultStatistics& statistics, double startFrequency, double endFrequency)
		{
			statistics = getFrequencyRangeStatistics(startFrequency, endFrequency);
		}
		
		const BaselineStatisticsMap &BaselineStatistics() const
		{
			if(_baselineStatistics.size() == 1)
				return _baselineStatistics.begin()->second;
			else if(_baselineStatistics.size() == 0)
				return _emptyBaselineStatisticsMap;
			else
				throw std::runtime_error("Requesting single band single baseline statistics in statistics collection with multiple bands");
		}
		
		const std::map<double, DefaultStatistics> &TimeStatistics() const
		{
			if(_timeStatistics.size() == 1)
				return _timeStatistics.begin()->second;
			else
				throw std::runtime_error("Requesting single band single timestep statistics in statistics collection with multiple bands");
		}
		
		const std::map<double, std::map<double, DefaultStatistics> > &AllTimeStatistics() const
		{
			return _timeStatistics;
		}
		
		const std::map<double, DefaultStatistics> &FrequencyStatistics() const
		{
			return _frequencyStatistics;
		}
		
		unsigned PolarizationCount() const
		{
			return _polarizationCount;
		}
		
		void SetPolarizationCount(unsigned newCount)
		{
			if(newCount != _polarizationCount)
			{
				_polarizationCount = newCount;
				_emptyBaselineStatisticsMap = BaselineStatisticsMap(_polarizationCount);
				_timeStatistics.clear();
				_frequencyStatistics.clear();
				_baselineStatistics.clear();
			}
		}
		
		virtual void Serialize(std::ostream &stream) const
		{
			SerializeToUInt64(stream, _polarizationCount);
			serializeTime(stream);
			serializeFrequency(stream);
			serializeBaselines(stream);
		}
		
		virtual void Unserialize(std::istream &stream)
		{
			_polarizationCount = UnserializeUInt64(stream);
			_emptyBaselineStatisticsMap = BaselineStatisticsMap(_polarizationCount);
			unserializeTime(stream);
			unserializeFrequency(stream);
			unserializeBaselines(stream);
		}
		
		void IntegrateBaselinesToOneChannel()
		{
			const size_t size = _baselineStatistics.size();
			if(size > 1)
			{
				BaselineStatisticsMap fullMap(_polarizationCount);
				double frequencySum = 0.0;
				
				for(std::map<double, BaselineStatisticsMap>::const_iterator i=_baselineStatistics.begin();i!=_baselineStatistics.end();++i)
				{
					frequencySum += i->first;
					fullMap += i->second;
				}
				
				_baselineStatistics.clear();
				_baselineStatistics.insert(std::pair<double, BaselineStatisticsMap>(frequencySum/size, fullMap));
			}
		}
		
		void IntegrateTimeToOneChannel()
		{
			const size_t size = _timeStatistics.size();
			if(size > 1)
			{
				DoubleStatMap fullMap;
				double frequencySum = 0.0;
				
				for(std::map<double, DoubleStatMap>::const_iterator i=_timeStatistics.begin();i!=_timeStatistics.end();++i)
				{
					frequencySum += i->first;
					addToDoubleStatMap(fullMap, i->second);
				}
				
				_timeStatistics.clear();
				_timeStatistics.insert(std::pair<double, DoubleStatMap>(frequencySum/size, fullMap));
			}
		}
		
		void LowerTimeResolution(size_t maxSteps)
		{
			for(std::map<double, DoubleStatMap>::iterator i=_timeStatistics.begin();i!=_timeStatistics.end();++i)
			{
				lowerResolution(i->second, maxSteps);
			}
		}
		
		void LowerFrequencyResolution(size_t maxSteps)
		{
			lowerResolution(_frequencyStatistics, maxSteps);
		}
		
		/**
		 * The regrid method will force all channels(/sub-bands) inside the collection to have the same
		 * uniform grid. It will do this by moving around time steps, using the first grid as reference.
		 * This is useful for raw (not NDPPP-ed) data, that might contain slightly different time steps in the
		 * different sub-bands, but are otherwise similarly gridded.
		 */
		void RegridTime()
		{
			if(_timeStatistics.size() > 1)
			{
				std::map<double, DoubleStatMap>::iterator i = _timeStatistics.begin();
				const DoubleStatMap &referenceMap = i->second;
				++i;
				do {
					regrid(referenceMap, i->second);
					++i;
				} while(i != _timeStatistics.end());
			}
		}
	private:
		struct StatisticSaver
		{
			QualityTablesFormatter::StatisticDimension dimension;
			double time;
			double frequency;
			unsigned antenna1;
			unsigned antenna2;
			QualityTablesFormatter *qualityData;
			
			void Save(StatisticalValue &value, unsigned kindIndex)
			{
				value.SetKindIndex(kindIndex);
				switch(dimension)
				{
					case QualityTablesFormatter::TimeDimension:
						qualityData->StoreTimeValue(time, frequency, value);
						break;
					case QualityTablesFormatter::FrequencyDimension:
						qualityData->StoreFrequencyValue(frequency, value);
						break;
					case QualityTablesFormatter::BaselineDimension:
						qualityData->StoreBaselineValue(antenna1, antenna2, frequency, value);
						break;
					case QualityTablesFormatter::BaselineTimeDimension:
						qualityData->StoreBaselineTimeValue(antenna1, antenna2, time, frequency, value);
						break;
				}
			}
		};
		
		struct Indices
		{
			unsigned kindRFICount;
			unsigned kindCount;
			unsigned kindSum;
			unsigned kindSumP2;
			unsigned kindDCount;
			unsigned kindDSum;
			unsigned kindDSumP2;
			
			void fill(QualityTablesFormatter &qd)
			{
				kindRFICount = qd.StoreOrQueryKindIndex(QualityTablesFormatter::RFICountStatistic),
				kindCount = qd.StoreOrQueryKindIndex(QualityTablesFormatter::CountStatistic),
				kindSum = qd.StoreOrQueryKindIndex(QualityTablesFormatter::SumStatistic),
				kindSumP2 = qd.StoreOrQueryKindIndex(QualityTablesFormatter::SumP2Statistic),
				kindDCount = qd.StoreOrQueryKindIndex(QualityTablesFormatter::DCountStatistic),
				kindDSum = qd.StoreOrQueryKindIndex(QualityTablesFormatter::DSumStatistic),
				kindDSumP2 = qd.StoreOrQueryKindIndex(QualityTablesFormatter::DSumP2Statistic);
			}
		};
		
		template<bool IsDiff>
		void addTimeAndBaseline(unsigned antenna1, unsigned antenna2, double time, double centralFrequency, int polarization, const float *reals, const float *imags, const bool *isRFI, const bool* origFlags, unsigned nsamples, unsigned step, unsigned stepRFI, unsigned stepFlags);
		
		template<bool IsDiff>
		void addToTimeFrequency(double time, const double* frequencies, int polarization, const float *reals, const float *imags, const bool *isRFI, const bool* origFlags, unsigned nsamples, unsigned step, unsigned stepRFI, unsigned stepFlags, bool shiftOneUp);
		
		template<bool IsDiff>
		void addToStatistic(DefaultStatistics &statistic, unsigned polarization, unsigned long count, long double sum_R, long double sum_I, long double sumP2_R, long double sumP2_I, unsigned long rfiCount)
		{
			if(IsDiff)
			{
				statistic.dCount[polarization] += count;
				statistic.dSum[polarization] += std::complex<long double>(sum_R, sum_I);
				statistic.dSumP2[polarization] += std::complex<long double>(sumP2_R, sumP2_I);
			} else {
				statistic.count[polarization] += count;
				statistic.sum[polarization] += std::complex<long double>(sum_R, sum_I);
				statistic.sumP2[polarization] += std::complex<long double>(sumP2_R, sumP2_I);
				statistic.rfiCount[polarization] += rfiCount;
			}
		}
		template<bool IsDiff>
		void addSingleNonRFISampleToStatistic(DefaultStatistics &statistic, unsigned polarization, long double sum_R, long double sum_I, long double sumP2_R, long double sumP2_I)
		{
			if(IsDiff)
			{
				++statistic.dCount[polarization];
				statistic.dSum[polarization] += std::complex<long double>(sum_R, sum_I);
				statistic.dSumP2[polarization] += std::complex<long double>(sumP2_R, sumP2_I);
			} else {
				++statistic.count[polarization];
				statistic.sum[polarization] += std::complex<long double>(sum_R, sum_I);
				statistic.sumP2[polarization] += std::complex<long double>(sumP2_R, sumP2_I);
			}
		}
		
		template<bool IsDiff>
		void addFrequency(unsigned band, int polarization, const float *reals, const float *imags, const bool *isRFI, const bool *origFlags, unsigned nsamples, unsigned step, unsigned stepRFI, unsigned stepFlags, bool shiftOneUp);
		
		void initializeEmptyStatistics(QualityTablesFormatter &qualityData, QualityTablesFormatter::StatisticDimension dimension) const
		{
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::RFICountStatistic, _polarizationCount);
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::CountStatistic, _polarizationCount);
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::SumStatistic, _polarizationCount);
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::SumP2Statistic, _polarizationCount);
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::DCountStatistic, _polarizationCount);
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::DSumStatistic, _polarizationCount);
			qualityData.InitializeEmptyStatistic(dimension, QualityTablesFormatter::DSumP2Statistic, _polarizationCount);
		}
		
		void saveEachStatistic(StatisticSaver &saver, const DefaultStatistics &stat, const Indices &indices) const
		{
			StatisticalValue value(_polarizationCount);
			
			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, std::complex<float>(stat.rfiCount[p], 0.0f));
			saver.Save(value, indices.kindRFICount);
			
			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, std::complex<float>(stat.count[p], 0.0f));
			saver.Save(value, indices.kindCount);

			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, stat.Sum<float>(p));
			saver.Save(value, indices.kindSum);

			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, stat.SumP2<float>(p));
			saver.Save(value, indices.kindSumP2);
			
			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, std::complex<float>(stat.dCount[p], 0.0f));
			saver.Save(value, indices.kindDCount);
			
			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, stat.DSum<float>(p));
			saver.Save(value, indices.kindDSum);

			for(unsigned p=0;p<_polarizationCount;++p)
				value.SetValue(p, stat.DSumP2<float>(p));
			saver.Save(value, indices.kindDSumP2);
		}
		
		void saveTime(QualityTablesFormatter &qd) const;
		
		void saveFrequency(QualityTablesFormatter &qd) const;
		
		void saveBaseline(QualityTablesFormatter &qd) const;
		
		DefaultStatistics &getTimeStatistic(double time, double centralFrequency)
		{
			// We use find() to see if the value exists, and only use insert() when it does not,
			// because insert is slow (because a "Statistic" needs to be created). Holds for both
			// frequency and time maps.
			std::map<double, DoubleStatMap>::iterator i = _timeStatistics.find(centralFrequency);
			if(i == _timeStatistics.end())
			{
				i = _timeStatistics.insert(std::pair<double, DoubleStatMap>(centralFrequency, DoubleStatMap())).first;
			}
			DoubleStatMap &selectedTimeStatistic = i->second;
			
			return getDoubleStatMapStatistic(selectedTimeStatistic, time);
		}
		
		DefaultStatistics &getFrequencyStatistic(double frequency)
		{
			return getDoubleStatMapStatistic(_frequencyStatistics, frequency);
		}
		
		DefaultStatistics &getDoubleStatMapStatistic(DoubleStatMap &map, double key)
		{
			// Use insert() only when not exist, as it is slower then find because a
			// Statistic is created.
			DoubleStatMap::iterator i = map.find(key);
			if(i == map.end())
			{
				i = map.insert(std::pair<double, DefaultStatistics>(key, DefaultStatistics(_polarizationCount))).first;
			}
			return i->second;
		}
		
		DefaultStatistics &getBaselineStatistic(unsigned antenna1, unsigned antenna2, double centralFrequency)
		{
			std::map<double, BaselineStatisticsMap>::iterator i = _baselineStatistics.find(centralFrequency);
			if(i == _baselineStatistics.end())
			{
				i = _baselineStatistics.insert(std::pair<double, BaselineStatisticsMap>(centralFrequency, BaselineStatisticsMap(_polarizationCount))).first;
			}
			BaselineStatisticsMap &selectedBaselineStatistic = i->second;
			return selectedBaselineStatistic.GetStatistics(antenna1, antenna2);
		}
		
		template<bool PerformAdd, typename T>
		void assignOrAdd(T &value, const T otherValue)
		{
			if(PerformAdd)
				value += otherValue;
			else
				value = otherValue;
		}
		
		template<bool AddStatistics>
		void assignStatistic(DefaultStatistics &destination, const StatisticalValue &source, QualityTablesFormatter::StatisticKind kind)
		{
			for(unsigned p=0;p<_polarizationCount;++p)
			{
				switch(kind)
				{
					case QualityTablesFormatter::RFICountStatistic:
						assignOrAdd<AddStatistics>(destination.rfiCount[p] , (long unsigned) source.Value(p).real());
						break;
					case QualityTablesFormatter::CountStatistic:
						assignOrAdd<AddStatistics>(destination.count[p] , (long unsigned) source.Value(p).real());
						break;
					case QualityTablesFormatter::SumStatistic:
						assignOrAdd<AddStatistics>(destination.sum[p] , std::complex<long double>(source.Value(p).real(), source.Value(p).imag() ));
						break;
					case QualityTablesFormatter::SumP2Statistic:
						assignOrAdd<AddStatistics>(destination.sumP2[p] , std::complex<long double>(source.Value(p).real(), source.Value(p).imag() ));
						break;
					case QualityTablesFormatter::DCountStatistic:
						assignOrAdd<AddStatistics>(destination.dCount[p] , (long unsigned) source.Value(p).real());
						break;
					case QualityTablesFormatter::DSumStatistic:
						assignOrAdd<AddStatistics>(destination.dSum[p] , std::complex<long double>(source.Value(p).real(), source.Value(p).imag() ));
						break;
					case QualityTablesFormatter::DSumP2Statistic:
						assignOrAdd<AddStatistics>(destination.dSumP2[p] , std::complex<long double>(source.Value(p).real(), source.Value(p).imag() ));
						break;
					default:
						break;
				}
			}
		}
		
		void forEachDefaultStatistic(QualityTablesFormatter &qd, void (StatisticsCollection::*functionName)(QualityTablesFormatter &, QualityTablesFormatter::StatisticKind))
		{
			(this->*functionName)(qd, QualityTablesFormatter::CountStatistic);
			(this->*functionName)(qd, QualityTablesFormatter::SumStatistic);
			(this->*functionName)(qd, QualityTablesFormatter::SumP2Statistic);
			(this->*functionName)(qd, QualityTablesFormatter::DCountStatistic);
			(this->*functionName)(qd, QualityTablesFormatter::DSumStatistic);
			(this->*functionName)(qd, QualityTablesFormatter::DSumP2Statistic);
			(this->*functionName)(qd, QualityTablesFormatter::RFICountStatistic);
		}
		
		template<bool AddStatistics>
		void loadSingleTimeStatistic(QualityTablesFormatter &qd, QualityTablesFormatter::StatisticKind kind)
		{
			std::vector<std::pair<QualityTablesFormatter::TimePosition, StatisticalValue> > values;
			unsigned kindIndex = qd.QueryKindIndex(kind);
			qd.QueryTimeStatistic(kindIndex, values);
			for(std::vector<std::pair<QualityTablesFormatter::TimePosition, StatisticalValue> >::const_iterator i=values.begin();i!=values.end();++i)
			{
				const QualityTablesFormatter::TimePosition &position = i->first;
				const StatisticalValue &statValue = i->second;
				
				DefaultStatistics &stat = getTimeStatistic(position.time, position.frequency);
				assignStatistic<AddStatistics>(stat, statValue, kind);
			}
		}
		
		template<bool AddStatistics>
		void loadTime(QualityTablesFormatter &qd)
		{
			forEachDefaultStatistic(qd, &StatisticsCollection::loadSingleTimeStatistic<AddStatistics>);
		}
		
		template<bool AddStatistics>
		void loadSingleFrequencyStatistic(QualityTablesFormatter &qd, QualityTablesFormatter::StatisticKind kind)
		{
			std::vector<std::pair<QualityTablesFormatter::FrequencyPosition, StatisticalValue> > values;
			unsigned kindIndex = qd.QueryKindIndex(kind);
			qd.QueryFrequencyStatistic(kindIndex, values);
			for(std::vector<std::pair<QualityTablesFormatter::FrequencyPosition, StatisticalValue> >::const_iterator i=values.begin();i!=values.end();++i)
			{
				const QualityTablesFormatter::FrequencyPosition &position = i->first;
				const StatisticalValue &statValue = i->second;
				
				DefaultStatistics &stat = getFrequencyStatistic(position.frequency);
				assignStatistic<AddStatistics>(stat, statValue, kind);
			}
		}
		
		template<bool AddStatistics>
		void loadFrequency(QualityTablesFormatter &qd)
		{
			forEachDefaultStatistic(qd, &StatisticsCollection::loadSingleFrequencyStatistic<AddStatistics>);
		}
		
		template<bool AddStatistics>
		void loadSingleBaselineStatistic(QualityTablesFormatter &qd, QualityTablesFormatter::StatisticKind kind)
		{
			std::vector<std::pair<QualityTablesFormatter::BaselinePosition, StatisticalValue> > values;
			unsigned kindIndex = qd.QueryKindIndex(kind);
			qd.QueryBaselineStatistic(kindIndex, values);
			for(std::vector<std::pair<QualityTablesFormatter::BaselinePosition, StatisticalValue> >::const_iterator i=values.begin();i!=values.end();++i)
			{
				const QualityTablesFormatter::BaselinePosition &position = i->first;
				const StatisticalValue &statValue = i->second;
				
				DefaultStatistics &stat = getBaselineStatistic(position.antenna1, position.antenna2, position.frequency);
				assignStatistic<AddStatistics>(stat, statValue, kind);
			}
		}
		
		template<bool AddStatistics>
		void loadBaseline(QualityTablesFormatter &qd)
		{
			forEachDefaultStatistic(qd, &StatisticsCollection::loadSingleBaselineStatistic<AddStatistics>);
		}
		
		double centralFrequency() const
		{
			double min =_frequencyStatistics.begin()->first;
			double max = _frequencyStatistics.rbegin()->first;
			return (min + max) / 2.0;
		}
		
		DefaultStatistics getGlobalStatistics(const DoubleStatMap &statMap) const
		{
			DefaultStatistics global(_polarizationCount);
			for(DoubleStatMap::const_iterator i=statMap.begin();i!=statMap.end();++i)
			{
				const DefaultStatistics &stat = i->second;
				global += stat;
			}
			return global;
		}
		
		DefaultStatistics getGlobalStatistics(const std::map<double, DoubleStatMap> &statMap) const
		{
			DefaultStatistics global(_polarizationCount);
			for(std::map<double, DoubleStatMap>::const_iterator i=statMap.begin();i!=statMap.end();++i)
			{
				const DefaultStatistics &stat = getGlobalStatistics(i->second);
				global += stat;
			}
			return global;
		}
		
		template<bool AutoCorrelations>
		DefaultStatistics getGlobalBaselineStatistics() const
		{
			DefaultStatistics global(_polarizationCount);
			
			for(std::map<double, BaselineStatisticsMap>::const_iterator f=_baselineStatistics.begin();f!=_baselineStatistics.end();++f)
			{
				const BaselineStatisticsMap &map = f->second;
				const std::vector<std::pair<unsigned, unsigned> > baselines = map.BaselineList();
				
				for(std::vector<std::pair<unsigned, unsigned> >::const_iterator i=baselines.begin();i!=baselines.end();++i)
				{
					const unsigned
						antenna1 = i->first,
						antenna2 =  i->second;
					if( ((antenna1 == antenna2) && AutoCorrelations) || ((antenna1 != antenna2) && (!AutoCorrelations)))
					{
						const DefaultStatistics &stat = map.GetStatistics(antenna1, antenna2);
						global += stat;
					}
				}
			}
			return global;
		}
		
		DefaultStatistics getFrequencyRangeStatistics(double startFrequency, double endFrequency) const
		{
			DefaultStatistics rangeStats(_polarizationCount);
			
			for(DoubleStatMap::const_iterator f=_frequencyStatistics.begin();f!=_frequencyStatistics.end();++f)
			{
				const double frequency = f->first;
				const DefaultStatistics &stat = f->second;
				
				if(frequency>=startFrequency && frequency < endFrequency)
					rangeStats += stat;
			}
			return rangeStats;
		}
		
		void serializeTime(std::ostream &stream) const
		{
			SerializeToUInt64(stream, _timeStatistics.size());
			
			for(std::map<double, DoubleStatMap>::const_iterator i=_timeStatistics.begin();i!=_timeStatistics.end();++i)
			{
				const double frequency = i->first;
				const DoubleStatMap &map = i->second;
				
				SerializeToDouble(stream, frequency);
				serializeDoubleStatMap(stream, map);
			}
		}
		
		void unserializeTime(std::istream &stream)
		{
			_timeStatistics.clear();
			size_t count = (size_t) UnserializeUInt64(stream);
			
			std::map<double, DoubleStatMap>::iterator insertPos = _timeStatistics.begin();
			for(size_t i=0;i<count;++i)
			{
				double frequency = UnserializeDouble(stream);
				insertPos =
					_timeStatistics.insert(insertPos, std::pair<double, DoubleStatMap>(frequency, DoubleStatMap()));
				unserializeDoubleStatMap(stream, insertPos->second);
			}
		}
		
		void serializeFrequency(std::ostream &stream) const
		{
			serializeDoubleStatMap(stream, _frequencyStatistics);
		}
		
		void unserializeFrequency(std::istream &stream)
		{
			_frequencyStatistics.clear();
			unserializeDoubleStatMap(stream, _frequencyStatistics);
		}
		
		void serializeBaselines(std::ostream &stream) const
		{
			SerializeToUInt64(stream, _baselineStatistics.size());
			
			for(std::map<double, BaselineStatisticsMap>::const_iterator i=_baselineStatistics.begin();i!=_baselineStatistics.end();++i)
			{
				const double frequency = i->first;
				const BaselineStatisticsMap &map = i->second;
				
				SerializeToDouble(stream, frequency);
				map.Serialize(stream);
			}
		}
		
		void unserializeBaselines(std::istream &stream)
		{
			_baselineStatistics.clear();
			size_t count = (size_t) UnserializeUInt64(stream);
			
			std::map<double, BaselineStatisticsMap>::iterator insertPos = _baselineStatistics.begin();
			for(size_t i=0;i<count;++i)
			{
				double frequency = UnserializeDouble(stream);
				insertPos = _baselineStatistics.insert(
					insertPos, std::pair<double, BaselineStatisticsMap>(frequency, BaselineStatisticsMap(_polarizationCount)));
				insertPos->second.Unserialize(stream);
			}
		}
		
		void serializeDoubleStatMap(std::ostream &stream, const DoubleStatMap &statMap) const
		{
			uint64_t statCount = statMap.size();
			stream.write(reinterpret_cast<char *>(&statCount), sizeof(statCount));
			for(DoubleStatMap::const_iterator i=statMap.begin();i!=statMap.end();++i)
			{
				const double &key = i->first;
				const DefaultStatistics &stat = i->second;
				stream.write(reinterpret_cast<const char *>(&key), sizeof(key));
				stat.Serialize(stream);
			}
		}
		
		void unserializeDoubleStatMap(std::istream &stream, DoubleStatMap &statMap) const
		{
			size_t count = (size_t) UnserializeUInt64(stream);
			
			std::map<double, DefaultStatistics>::iterator insertPos = statMap.begin();
			for(size_t i=0;i<count;++i)
			{
				double key = UnserializeDouble(stream);
				insertPos =
					statMap.insert(insertPos, std::pair<double, DefaultStatistics>(key, DefaultStatistics(_polarizationCount)));
				insertPos->second.Unserialize(stream);
			}
		}
		
		void addTime(const StatisticsCollection &collection)
		{
			for(std::map<double, DoubleStatMap>::const_iterator i=collection._timeStatistics.begin();i!=collection._timeStatistics.end();++i)
			{
				const double frequency = i->first;
				const DoubleStatMap &map = i->second;
				
				for(DoubleStatMap::const_iterator j=map.begin();j!=map.end();++j)
				{
					const double time = j->first;
					const DefaultStatistics &stat = j->second;
					getTimeStatistic(time, frequency) += stat;
				}
			}
		}
		
		void addFrequency(const StatisticsCollection &collection)
		{
			for(DoubleStatMap::const_iterator j=collection._frequencyStatistics.begin();j!=collection._frequencyStatistics.end();++j)
			{
				const double frequency = j->first;
				const DefaultStatistics &stat = j->second;
				getFrequencyStatistic(frequency) += stat;
			}
		}
		
		void addBaseline(const StatisticsCollection &collection)
		{
			for(std::map<double, BaselineStatisticsMap>::const_iterator i=collection._baselineStatistics.begin();i!=collection._baselineStatistics.end();++i)
			{
				const double frequency = i->first;
				const BaselineStatisticsMap &map = i->second;
				
				vector<std::pair<unsigned, unsigned> > baselines = map.BaselineList();
				for(vector<std::pair<unsigned, unsigned> >::const_iterator j=baselines.begin();j!=baselines.end();++j)
				{
					const unsigned antenna1 = j->first;
					const unsigned antenna2 = j->second;
					const DefaultStatistics &stat = map.GetStatistics(antenna1, antenna2);
					getBaselineStatistic(antenna1, antenna2, frequency) += stat;
				}
			}
		}
		
		void addToDoubleStatMap(DoubleStatMap &dest, const DoubleStatMap &source)
		{
			for(DoubleStatMap::const_iterator i=source.begin();i!=source.end();++i)
			{
				double key = i->first;
				const DefaultStatistics &sourceStats = i->second;
				
				getDoubleStatMapStatistic(dest, key) += sourceStats;
			}
		}
		
		void lowerResolution(DoubleStatMap &map, size_t maxSteps) const;
		
		static void regrid(const DoubleStatMap &referenceMap, DoubleStatMap &regridMap)
		{
			DoubleStatMap newMap;
			for(DoubleStatMap::const_iterator i=regridMap.begin();i!=regridMap.end();++i)
			{
				double key = i->first;
				
				// find the key in the reference map that is closest to this key, if it is within range
				DoubleStatMap::const_iterator bound = referenceMap.lower_bound(key);
				if(bound != referenceMap.end())
				{
					double rightKey = bound->first;
					if(bound != referenceMap.begin())
					{
						--bound;
						double leftKey = bound->first;
						if(key - rightKey < leftKey - key)
							key = rightKey;
						else
							key = leftKey;
					}
				}
				newMap.insert(std::pair<double, DefaultStatistics>(key, i->second));
			}
			regridMap = newMap;
		}
		
		std::map<double, DoubleStatMap> _timeStatistics;
		DoubleStatMap _frequencyStatistics;
		std::map<double, BaselineStatisticsMap> _baselineStatistics;
		
		std::map<unsigned, std::vector< DefaultStatistics *> > _bands;
		std::map<unsigned, double> _centralFrequencies;
		std::map<unsigned, std::vector<double> > _bandFrequencies;
		
		unsigned _polarizationCount;
		BaselineStatisticsMap _emptyBaselineStatisticsMap;
};

#endif
