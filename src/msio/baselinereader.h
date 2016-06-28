#ifndef BASELINEREADER_H
#define BASELINEREADER_H

#include <map>
#include <vector>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

#include "../structures/antennainfo.h"
#include "../structures/image2d.h"
#include "../structures/mask2d.h"
#include "../structures/measurementset.h"

typedef boost::shared_ptr<class BaselineReader> BaselineReaderPtr;
typedef boost::shared_ptr<const class BaselineReader> BaselineReaderCPtr;

class BaselineReader {
	public:
		BaselineReader(const std::string &msFile);
		virtual ~BaselineReader();

		bool ReadFlags() const { return _readFlags; }
		void SetReadFlags(bool readFlags) { _readFlags = readFlags; }

		bool ReadData() const { return _readData; }
		void SetReadData(bool readData) { _readData = readData; }

		const std::string &DataColumnName() const { return _dataColumnName; }
		void SetDataColumnName(const std::string &name) { _dataColumnName = name; }

		bool SubtractModel() const { return _subtractModel; }
		void SetSubtractModel(bool subtractModel) { _subtractModel = subtractModel; }

		size_t PolarizationCount()
		{
			initializePolarizations();
			return _polarizationCount;
		}

		class casacore::MeasurementSet *Table() const { return _table; }

		MeasurementSet &Set() { return _measurementSet; }

		const std::map<double,size_t> &ObservationTimes(size_t sequenceId) const
		{ 
			return _observationTimes[sequenceId];
		}
		
		std::vector<double> ObservationTimes(size_t startIndex, size_t endIndex) const {
			std::vector<double> times;
			times.insert(times.begin(), _observationTimesVector.begin()+startIndex, _observationTimesVector.begin()+endIndex);
			return times;
		}

		void AddReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow, size_t sequenceId);
		void AddReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow, size_t sequenceId, size_t startIndex, size_t endIndex)
		{
			addReadRequest(antenna1, antenna2, spectralWindow, sequenceId, startIndex, endIndex);
		}
		virtual void PerformReadRequests() = 0;
		
		void AddWriteTask(std::vector<Mask2DCPtr> flags, int antenna1, int antenna2, int spectralWindow, unsigned sequenceId)
		{
			initializePolarizations();
			if(flags.size() != _polarizationCount)
			{
				std::stringstream s;
				s << "Trying to write image with " << flags.size() << " polarizations to a measurement set with " << _polarizationCount;
				throw std::runtime_error(s.str());
			}
			FlagWriteRequest task;
			task.flags = flags;
			task.antenna1 = antenna1;
			task.antenna2 = antenna2;
			task.spectralWindow = spectralWindow;
			task.sequenceId = sequenceId;
			task.startIndex = 0;
			task.endIndex = flags[0]->Width();
			task.leftBorder = 0;
			task.rightBorder = 0;
			_writeRequests.push_back(task);
		}
		virtual void PerformFlagWriteRequests() = 0;
		virtual void PerformDataWriteTask(std::vector<Image2DCPtr> _realImages, std::vector<Image2DCPtr> _imaginaryImages, int antenna1, int antenna2, int spectralWindow, unsigned sequenceId) = 0;
		
		class TimeFrequencyData GetNextResult(std::vector<class UVW> &uvw);

		virtual size_t GetMinRecommendedBufferSize(size_t threadCount) { return threadCount; }
		virtual size_t GetMaxRecommendedBufferSize(size_t threadCount) { return 2*threadCount; }
		
		static uint64_t MeasurementSetDataSize(const std::string &filename);
	protected:
		struct ReadRequest {
			int antenna1;
			int antenna2;
			int spectralWindow;
			unsigned sequenceId;
			size_t startIndex;
			size_t endIndex;
		};
		struct FlagWriteRequest {
			FlagWriteRequest() { }
			FlagWriteRequest(const FlagWriteRequest &source)
			: flags(source.flags), antenna1(source.antenna1), antenna2(source.antenna2), spectralWindow(source.spectralWindow), sequenceId(source.sequenceId),
			startIndex(source.startIndex), endIndex(source.endIndex),
			leftBorder(source.leftBorder), rightBorder(source.rightBorder)
			{
			}
			void operator=(const FlagWriteRequest &source)
			{
				flags = source.flags;
				antenna1 = source.antenna1;
				antenna2 = source.antenna2;
				spectralWindow = source.spectralWindow;
				sequenceId = source.sequenceId;
				startIndex = source.startIndex;
				endIndex = source.endIndex;
				leftBorder = source.leftBorder;
				rightBorder = source.rightBorder;
			}
			std::vector<Mask2DCPtr> flags;
			int antenna1;
			int antenna2;
			int spectralWindow;
			unsigned sequenceId;
			size_t startIndex;
			size_t endIndex;
			size_t leftBorder;
			size_t rightBorder;
		};
		
		struct Result {
			Result() { }
			Result(const Result &source) :
				_realImages(source._realImages),
				_imaginaryImages(source._imaginaryImages),
				_flags(source._flags),
				_uvw(source._uvw),
				_bandInfo(source._bandInfo)
				{
				}
			void operator=(const Result &source)
			{
				_realImages = source._realImages;
				_imaginaryImages = source._imaginaryImages;
				_flags = source._flags;
				_uvw = source._uvw;
				_bandInfo = source._bandInfo;
			}
			std::vector<Image2DPtr> _realImages;
			std::vector<Image2DPtr> _imaginaryImages;
			std::vector<Mask2DPtr> _flags;
			std::vector<class UVW> _uvw;
			class BandInfo _bandInfo;
		};
		void initializeMeta()
		{
			initObservationTimes();
			initializePolarizations();
		}
		//casacore::ROArrayColumn<casacore::Complex> *CreateDataColumn(const std::string &columnName, class casacore::Table &table);
		//casacore::ArrayColumn<casacore::Complex> *CreateDataColumnRW(const std::string &columnName, class casacore::Table &table);

		std::vector<ReadRequest> _readRequests;
		std::vector<FlagWriteRequest> _writeRequests;
		std::vector<Result> _results;
	private:
		void initializePolarizations();
		void initObservationTimes();
		
		void addReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow, size_t sequenceId, size_t startIndex, size_t endIndex)
		{
			ReadRequest request;
			request.antenna1 = antenna1;
			request.antenna2 = antenna2;
			request.spectralWindow = spectralWindow;
			request.sequenceId = sequenceId;
			request.startIndex = startIndex;
			request.endIndex = endIndex;
			_readRequests.push_back(request);
		}

		MeasurementSet _measurementSet;
		class casacore::MeasurementSet *_table;
		
		std::string _dataColumnName;
		bool _subtractModel;
		bool _readData, _readFlags;
		
		std::vector<std::map<double,size_t> > _observationTimes;
		std::vector<double> _observationTimesVector;
		size_t _polarizationCount;
};

#endif // BASELINEREADER_H
