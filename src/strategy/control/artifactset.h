#ifndef RFI_RFISTRATEGY_H
#define RFI_RFISTRATEGY_H 

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

#include "../../structures/types.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include "../../types.h"

#include "../algorithms/types.h"

#include "../control/types.h"

class UVImager;

namespace rfiStrategy {
	class	ArtifactSet
	{
		public:
			explicit ArtifactSet(std::mutex* ioMutex) :
			_visualizationData(),
			_metaData(),
			_sensitivity(1.0L),
			_projectedDirectionRad(0.0L),
			_data(new Data()),
			_imageSet(),
			_imageSetIndex(),
			_ioMutex(ioMutex),
			_horizontalProfile(),
			_verticalProfile()
			{ }

			ArtifactSet(const ArtifactSet& source) = default;
			ArtifactSet(ArtifactSet&& source) = default;
			~ArtifactSet();

			ArtifactSet &operator=(const ArtifactSet& source) = default;
			ArtifactSet &operator=(ArtifactSet&& source) = default;

			void SetOriginalData(const TimeFrequencyData &data)
			{
				_originalData = data;
			}

			void SetRevisedData(const TimeFrequencyData &data)
			{
				_revisedData = data;
			}

			void SetContaminatedData(const TimeFrequencyData &data)
			{
				_contaminatedData = data;
			}

			void SetSensitivity(numl_t sensitivity)
			{
				_sensitivity = sensitivity;
			}
			numl_t Sensitivity() const { return _sensitivity; }

			const TimeFrequencyData &OriginalData() const { return _originalData; }
			TimeFrequencyData &OriginalData() { return _originalData; }

			const TimeFrequencyData &RevisedData() const { return _revisedData; }
			TimeFrequencyData &RevisedData() { return _revisedData; }

			const TimeFrequencyData &ContaminatedData() const { return _contaminatedData; }
			TimeFrequencyData &ContaminatedData() { return _contaminatedData; }

			void SetCanVisualize(bool canVisualize) { _canVisualize = canVisualize; }
			void AddVisualization(const std::string& label, const TimeFrequencyData& data, size_t sortingIndex)
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
									// Can't merge, just add like normal
									_visualizationData.emplace_back(label, data, sortingIndex);
									return;
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
			const std::vector<std::tuple<std::string, TimeFrequencyData, size_t>>& Visualizations() const { return _visualizationData; }

			class ImageSet& ImageSet() const { return *_imageSet; }
			void SetImageSet(std::unique_ptr<class ImageSet> imageSet);
			void SetNoImageSet();
			
			class ImageSetIndex& ImageSetIndex() const { return *_imageSetIndex; }
			void SetImageSetIndex(std::unique_ptr<class ImageSetIndex> imageSetIndex);

			class UVImager& Imager() const { return *_imager; }
			void SetImager(class UVImager* imager);
			
			bool HasImageSet() const { return _imageSet != nullptr; }
			bool HasImageSetIndex() const { return _imageSetIndex != nullptr; }
			bool HasImager() const { return _imager != nullptr; }
			bool HasMetaData() const { return _metaData != nullptr; }
			
			TimeFrequencyMetaDataCPtr MetaData()
			{
				return _metaData;
			}
			void SetMetaData(TimeFrequencyMetaDataCPtr metaData)
			{
				_metaData = metaData;
			}

			std::mutex& IOMutex()
			{
				return *_ioMutex;
			}

			bool HasAntennaFlagCountPlot() const { return _data->_antennaFlagCountPlot!=nullptr; }
			class AntennaFlagCountPlot& AntennaFlagCountPlot()
			{
				return *_data->_antennaFlagCountPlot;
			}
			void SetAntennaFlagCountPlot(std::unique_ptr<class AntennaFlagCountPlot> plot);
			class FrequencyFlagCountPlot& FrequencyFlagCountPlot()
			{
				return *_data->_frequencyFlagCountPlot;
			}
			void SetFrequencyFlagCountPlot(std::unique_ptr<class FrequencyFlagCountPlot> plot);
			class FrequencyPowerPlot& FrequencyPowerPlot()
			{
				return *_data->_frequencyPowerPlot;
			}
			void SetFrequencyPowerPlot(std::unique_ptr<class FrequencyPowerPlot> plot);
			class TimeFlagCountPlot& TimeFlagCountPlot()
			{
				return *_data->_timeFlagCountPlot;
			}
			void SetTimeFlagCountPlot(std::unique_ptr<class TimeFlagCountPlot> plot);
			class PolarizationStatistics& PolarizationStatistics()
			{
				return *_data->_polarizationStatistics;
			}
			void SetPolarizationStatistics(std::unique_ptr<class PolarizationStatistics> statistics);
			class BaselineSelector& BaselineSelectionInfo()
			{
				return *_data->_baselineSelectionInfo;
			}
			bool HasIterationsPlot() const { return _data->_iterationsPlot!=nullptr; }
			void SetIterationsPlot(std::unique_ptr<class IterationsPlot> iterationsPlot);
			class IterationsPlot& IterationsPlot()
			{
				return *_data->_iterationsPlot;
			}
			void SetBaselineSelectionInfo(std::unique_ptr<class BaselineSelector> baselineSelectionInfo);
			void SetObservatorium(std::unique_ptr<class Observatorium> observatorium);
			class Observatorium& Observatorium() const
			{
				return *_data->_observatorium;
			}
			void SetModel(std::unique_ptr<class Model> model);
			class Model& Model() const
			{
				return *_data->_model;
			}
			void SetProjectedDirectionRad(numl_t projectedDirectionRad)
			{
				_projectedDirectionRad = projectedDirectionRad;
			}
			numl_t ProjectedDirectionRad() const
			{
				return _projectedDirectionRad;
			}
			const std::vector<num_t> &HorizontalProfile() const { return _horizontalProfile; }
			std::vector<num_t> &HorizontalProfile() { return _horizontalProfile; }
			
			const std::vector<num_t> &VerticalProfile() const { return _verticalProfile; }
			std::vector<num_t> &VerticalProfile() { return _verticalProfile; }
			
		private:
			TimeFrequencyData _originalData;
			TimeFrequencyData _contaminatedData;
			TimeFrequencyData _revisedData;
			bool _canVisualize;
			std::vector<std::tuple<std::string, TimeFrequencyData, size_t>> _visualizationData;
			TimeFrequencyMetaDataCPtr _metaData;
			numl_t _sensitivity;
			numl_t _projectedDirectionRad;
			class UVImager* _imager;
			
			struct Data {
				Data();
				~Data();
				std::unique_ptr<class AntennaFlagCountPlot> _antennaFlagCountPlot;
				std::unique_ptr<class FrequencyFlagCountPlot> _frequencyFlagCountPlot;
				std::unique_ptr<class FrequencyPowerPlot> _frequencyPowerPlot;
				std::unique_ptr<class TimeFlagCountPlot> _timeFlagCountPlot;
				std::unique_ptr<class IterationsPlot> _iterationsPlot;
				std::unique_ptr<class PolarizationStatistics> _polarizationStatistics;
				std::unique_ptr<class BaselineSelector> _baselineSelectionInfo;
				std::unique_ptr<class Observatorium> _observatorium;
				std::unique_ptr<class Model> _model;
			};
			
			std::shared_ptr<Data> _data;
			std::shared_ptr<class ImageSet> _imageSet;
			std::shared_ptr<class ImageSetIndex> _imageSetIndex;
			std::mutex *_ioMutex;
			
			std::vector<num_t> _horizontalProfile, _verticalProfile;
	};
}

#endif //RFI_RFISTRATEGY_H
