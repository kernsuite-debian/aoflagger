#include "directbaselinereader.h"

#include <vector>
#include <set>
#include <stdexcept>

#include <casacore/tables/DataMan/TiledStManAccessor.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include "../structures/arraycolumniterator.h"
#include "../structures/scalarcolumniterator.h"
#include "../structures/timefrequencydata.h"

#include "../util/aologger.h"
#include "../util/stopwatch.h"

DirectBaselineReader::DirectBaselineReader(const std::string &msFile) : BaselineReader(msFile)
{
}

DirectBaselineReader::~DirectBaselineReader()
{
	ShowStatistics();
}

void DirectBaselineReader::initBaselineCache()
{
	// Pass one time through the entire measurement set and store the rownumbers of
	// the baselines.
	if(_baselineCache.empty())
	{
		AOLogger::Debug << "Determining sequence positions within file for direct baseline reader...\n";
		std::vector<size_t> dataIdToSpw;
		Set().GetDataDescToBandVector(dataIdToSpw);
		
		casacore::ROScalarColumn<int> antenna1Column(*Table(), "ANTENNA1"); 
		casacore::ROScalarColumn<int> antenna2Column(*Table(), "ANTENNA2");
		casacore::ROScalarColumn<int> dataDescIdColumn(*Table(), "DATA_DESC_ID");
		casacore::ROScalarColumn<int> fieldIdColumn(*Table(), "FIELD_ID");
		
		int prevFieldId = -1, sequenceId = -1;
		for(size_t i=0;i<Table()->nrow();++i) {
			int
				antenna1 = antenna1Column(i),
				antenna2 = antenna2Column(i),
				dataDescId = dataDescIdColumn(i),
				fieldId = fieldIdColumn(i);
			if(fieldId != prevFieldId)
			{
				prevFieldId = fieldId;
				sequenceId++;
			}
			int spectralWindow = dataIdToSpw[dataDescId];
			addRowToBaselineCache(antenna1, antenna2, spectralWindow, sequenceId, i);
		}
	}
}

void DirectBaselineReader::addRowToBaselineCache(int antenna1, int antenna2, int spectralWindow, int sequenceId, size_t row)
{
	BaselineCacheIndex searchItem;
	searchItem.antenna1 = antenna1;
	searchItem.antenna2 = antenna2;
	searchItem.spectralWindow = spectralWindow;
	searchItem.sequenceId = sequenceId;
	std::map<BaselineCacheIndex,BaselineCacheValue>::iterator cacheItemIter = _baselineCache.find(searchItem);
	if(cacheItemIter == _baselineCache.end())
	{
		BaselineCacheValue cacheValue;
		cacheValue.rows.push_back(row);
		_baselineCache.insert(std::make_pair(searchItem, cacheValue));
	} else {
		cacheItemIter->second.rows.push_back(row);
	}
}

void DirectBaselineReader::addRequestRows(ReadRequest request, size_t requestIndex, std::vector<std::pair<size_t, size_t> > &rows)
{
	BaselineCacheIndex searchItem;
	searchItem.antenna1 = request.antenna1;
	searchItem.antenna2 = request.antenna2;
	searchItem.spectralWindow = request.spectralWindow;
	searchItem.sequenceId = request.sequenceId;
	std::map<BaselineCacheIndex,BaselineCacheValue>::iterator cacheItemIter = _baselineCache.find(searchItem);
	if(cacheItemIter != _baselineCache.end())
	{
		const std::vector<size_t> &cacheRows = cacheItemIter->second.rows;
		for(std::vector<size_t>::const_iterator j=cacheRows.begin(); j!=cacheRows.end(); ++j)
			rows.push_back(std::pair<size_t, size_t>(*j, requestIndex));
	}
}

void DirectBaselineReader::addRequestRows(FlagWriteRequest request, size_t requestIndex, std::vector<std::pair<size_t, size_t> > &rows)
{
	BaselineCacheIndex searchItem;
	searchItem.antenna1 = request.antenna1;
	searchItem.antenna2 = request.antenna2;
	searchItem.spectralWindow = request.spectralWindow;
	searchItem.sequenceId = request.sequenceId;
	std::map<BaselineCacheIndex,BaselineCacheValue>::iterator cacheItemIter = _baselineCache.find(searchItem);
	if(cacheItemIter != _baselineCache.end())
	{
		const std::vector<size_t> &cacheRows = cacheItemIter->second.rows;
		for(std::vector<size_t>::const_iterator j=cacheRows.begin();j!=cacheRows.end();++j)
			rows.push_back(std::pair<size_t, size_t>(*j, requestIndex));
	}
}

void DirectBaselineReader::PerformReadRequests()
{
  Stopwatch stopwatch(true);
	
	initializeMeta();
	initBaselineCache();

	// Each element contains (row number, corresponding request index)
	std::vector<std::pair<size_t, size_t> > rows;
	
	for(size_t i=0;i!=_readRequests.size();++i)
		addRequestRows(_readRequests[i], i, rows);
	std::sort(rows.begin(), rows.end());
	
	AOLogger::Debug << "Reading " << _readRequests.size() << " requests with " << rows.size() << " rows total, flags=" << ReadFlags() << ", " << PolarizationCount() << " polarizations.\n";
	
	_results.clear();
	for(size_t i=0;i<_readRequests.size();++i)
	{
		_results.push_back(Result());
		size_t
			startIndex = _readRequests[i].startIndex,
			endIndex = _readRequests[i].endIndex,
			band = _readRequests[i].spectralWindow,
			channelCount = Set().FrequencyCount(band),
			sequenceId = _readRequests[i].sequenceId,
			timeCount = ObservationTimes(sequenceId).size();
			
		if(startIndex > timeCount)
		{
			AOLogger::Warn << "startIndex > timeCount\n";
		}
		if(endIndex > timeCount)
		{
			endIndex = timeCount;
			AOLogger::Warn << "endIndex (" << endIndex << ") > timeCount (" << timeCount << ")\n";
		}

		size_t width = endIndex-startIndex;
		for(size_t p=0;p<PolarizationCount();++p)
		{
			if(ReadData()) {
				_results[i]._realImages.push_back(Image2D::CreateZeroImagePtr(width, channelCount));
				_results[i]._imaginaryImages.push_back(Image2D::CreateZeroImagePtr(width, channelCount));
			}
			if(ReadFlags()) {
				// The flags should be initialized to true, as a baseline might
				// miss some time scans that other baselines do have, and these
				// should be flagged.
				_results[i]._flags.push_back(Mask2D::CreateSetMaskPtr<true>(width, channelCount));
			}
		}
		_results[i]._uvw.resize(width);
	}

	casacore::Table &table = *Table();

	casacore::ROScalarColumn<double> timeColumn(table, "TIME");
	casacore::ROArrayColumn<float> weightColumn(table, "WEIGHT");
	casacore::ROArrayColumn<double> uvwColumn(table, "UVW");
	casacore::ROArrayColumn<bool> flagColumn(table, "FLAG");
	casacore::ROArrayColumn<casacore::Complex> *modelColumn;

	casacore::ROArrayColumn<casacore::Complex> *dataColumn = 0;
	if(ReadData())
		dataColumn = new casacore::ROArrayColumn<casacore::Complex>(table, DataColumnName());

	if(SubtractModel()) {
		modelColumn = new casacore::ROArrayColumn<casacore::Complex>(table, "MODEL_DATA");
	} else {
		modelColumn = 0;
	}

	for(std::vector<std::pair<size_t, size_t> >::const_iterator i=rows.begin();i!=rows.end();++i) {
		size_t rowIndex = i->first;
		size_t requestIndex = i->second;
		
		double time = timeColumn(rowIndex);
		const ReadRequest &request = _readRequests[requestIndex];
		size_t
			timeIndex = ObservationTimes(request.sequenceId).find(time)->second,
			startIndex = request.startIndex,
			endIndex = request.endIndex,
			band = request.spectralWindow;
		bool timeIsSelected = timeIndex>=startIndex && timeIndex<endIndex;
		if(ReadData() && timeIsSelected) {
			if(DataKind() == WeightData)
				readWeights(requestIndex, timeIndex-startIndex, Set().FrequencyCount(band), weightColumn(rowIndex));
			else if(modelColumn == 0)
				readTimeData(requestIndex, timeIndex-startIndex, Set().FrequencyCount(band), (*dataColumn)(rowIndex), 0);
			else {
				const casacore::Array<casacore::Complex> model = (*modelColumn)(rowIndex); 
				readTimeData(requestIndex, timeIndex-startIndex, Set().FrequencyCount(band), (*dataColumn)(rowIndex), &model);
			}
		}
		if(ReadFlags() && timeIsSelected) {
			readTimeFlags(requestIndex, timeIndex-startIndex, Set().FrequencyCount(band), flagColumn(rowIndex));
		}
		if(timeIsSelected) {
			casacore::Array<double> arr = uvwColumn(rowIndex);
			casacore::Array<double>::const_iterator i = arr.begin();
			_results[requestIndex]._uvw[timeIndex-startIndex].u = *i;
			++i;
			_results[requestIndex]._uvw[timeIndex-startIndex].v = *i;
			++i;
			_results[requestIndex]._uvw[timeIndex-startIndex].w = *i;
		}
	}
	if(dataColumn != 0)
		delete dataColumn;
	
	AOLogger::Debug << "Time of ReadRequests(): " << stopwatch.ToString() << '\n';

	_readRequests.clear();
}

std::vector<UVW> DirectBaselineReader::ReadUVW(unsigned antenna1, unsigned antenna2, unsigned spectralWindow, unsigned sequenceId)
{
  Stopwatch stopwatch(true);
	
	initializeMeta();
	initBaselineCache();

	const std::map<double, size_t> &observationTimes = ObservationTimes(sequenceId);

	// Each element contains (row number, corresponding request index)
	std::vector<std::pair<size_t, size_t> > rows;
	ReadRequest request;
	request.antenna1 = antenna1;
	request.antenna2 = antenna2;
	request.spectralWindow = spectralWindow;
	request.sequenceId = sequenceId;
	request.startIndex = 0;
	request.endIndex = observationTimes.size();
	addRequestRows(request, 0, rows);
	std::sort(rows.begin(), rows.end());
	
	size_t width = observationTimes.size();

	casacore::Table &table = *Table();
	casacore::ROScalarColumn<double> timeColumn(table, "TIME");
	casacore::ROArrayColumn<double> uvwColumn(table, "UVW");
	
	std::vector<UVW> uvws;
	uvws.resize(width);

	for(std::vector<std::pair<size_t, size_t> >::const_iterator i=rows.begin();i!=rows.end();++i) {
		size_t rowIndex = i->first;
		
		double time = timeColumn(rowIndex);
		size_t
			timeIndex = observationTimes.find(time)->second;

		casacore::Array<double> arr = uvwColumn(rowIndex);
		casacore::Array<double>::const_iterator j = arr.begin();
		UVW &uvw = uvws[timeIndex];
		uvw.u = *j;
		++j;
		uvw.v = *j;
		++j;
		uvw.w = *j;
	}
	
	AOLogger::Debug << "Read of UVW took: " << stopwatch.ToString() << '\n';
	return uvws;
}

void DirectBaselineReader::PerformFlagWriteRequests()
{
	Stopwatch stopwatch(true);

	initializeMeta();

	initBaselineCache();

	// Each element contains (row number, corresponding request index)
	std::vector<std::pair<size_t, size_t> > rows;
	
	for(size_t i=0;i!=_writeRequests.size();++i)
		addRequestRows(_writeRequests[i], i, rows);
	std::sort(rows.begin(), rows.end());

	casacore::ROScalarColumn<double> timeColumn(*Table(), "TIME");
	casacore::ArrayColumn<bool> flagColumn(*Table(), "FLAG");

	for(std::vector<FlagWriteRequest>::iterator i=_writeRequests.begin();i!=_writeRequests.end();++i)
	{
		size_t band = i->spectralWindow;
		if(Set().FrequencyCount(band) != i->flags[0]->Height())
		{
			std::cerr << "The frequency count in the measurement set (" << Set().FrequencyCount(band) << ") does not match the image!" << std::endl;
		}
		if(i->endIndex - i->startIndex != i->flags[0]->Width())
		{
			std::cerr << "The number of time scans to write in the measurement set (" << (i->endIndex - i->startIndex) << ") does not match the image (" << i->flags[0]->Width() << ") !" << std::endl;
		}
	}

	size_t rowsWritten = 0;

	for(std::vector<std::pair<size_t, size_t> >::const_iterator i=rows.begin();i!=rows.end();++i)
	{
		size_t rowIndex = i->first;
		FlagWriteRequest &request = _writeRequests[i->second];
		double time = timeColumn(rowIndex);
		size_t timeIndex = ObservationTimes(request.sequenceId).find(time)->second;
		if(timeIndex >= request.startIndex + request.leftBorder && timeIndex < request.endIndex - request.rightBorder)
		{
			casacore::Array<bool> flag = flagColumn(rowIndex);
			casacore::Array<bool>::iterator j = flag.begin();
			for(size_t f=0;f<(size_t) Set().FrequencyCount(request.spectralWindow);++f) {
				for(size_t p=0;p<PolarizationCount();++p)
				{
					*j = request.flags[p]->Value(timeIndex - request.startIndex, f);
					++j;
				}
			}
			flagColumn.basePut(rowIndex, flag);
			++rowsWritten;
		}
	}
	_writeRequests.clear();
	
	AOLogger::Debug << rowsWritten << "/" << rows.size() << " rows written in " << stopwatch.ToString() << '\n';
}

void DirectBaselineReader::readTimeData(size_t requestIndex, size_t xOffset, int frequencyCount, const casacore::Array<casacore::Complex> data, const casacore::Array<casacore::Complex> *model)
{
	casacore::Array<casacore::Complex>::const_iterator i = data.begin();
	casacore::Array<casacore::Complex>::const_iterator m;
	if(DataKind() == ResidualData)
		m = model->begin();

	size_t polarizationCount = PolarizationCount();

	for(size_t f=0;f<(size_t) frequencyCount;++f) {
		num_t rv,iv;

		for(size_t p=0;p<polarizationCount;++p)
		{
			if(DataKind() == ResidualData)
			{
				const casacore::Complex &iData = *i;
				const casacore::Complex &iModel = *m;
				++i; ++m;

				rv = iData.real() - iModel.real();
				iv = iData.imag() - iModel.imag();
			} else {
				const casacore::Complex &complex = *i;
				++i;

				rv = complex.real();
				iv = complex.imag();
			}
			_results[requestIndex]._realImages[p]->SetValue(xOffset, f, rv);
			_results[requestIndex]._imaginaryImages[p]->SetValue(xOffset, f, iv);
		}
	}
}

void DirectBaselineReader::readTimeFlags(size_t requestIndex, size_t xOffset, int frequencyCount, const casacore::Array<bool> flag)
{
	size_t polarizationCount = PolarizationCount();

	casacore::Array<bool>::const_iterator j = flag.begin();
	for(size_t f=0;f<(size_t) frequencyCount;++f) {
		for(size_t p=0;p<polarizationCount;++p)
		{
			bool v = *j;
			++j;
			_results[requestIndex]._flags[p]->SetValue(xOffset, f, v);
		} 
	}
}

void DirectBaselineReader::readWeights(size_t requestIndex, size_t xOffset, int frequencyCount, const casacore::Array<float> weight)
{
	size_t polarizationCount = PolarizationCount();

	casacore::Array<float>::const_iterator j = weight.begin();
	std::vector<float> values(polarizationCount);
	for(size_t p=0;p<polarizationCount;++p) {
		values[p] = *j;
		++j;
	}
	for(size_t f=0;f<(size_t) frequencyCount;++f) {
		for(size_t p=0;p<polarizationCount;++p)
		{
			_results[requestIndex]._realImages[p]->SetValue(xOffset, f, values[p]);
			_results[requestIndex]._imaginaryImages[p]->SetValue(xOffset, f, 0.0);
		}
	} 
}

void DirectBaselineReader::ShowStatistics()
{
	try {
		casacore::ROTiledStManAccessor accessor(*Table(), "LofarStMan");
		std::stringstream s;
		accessor.showCacheStatistics(s);
		AOLogger::Debug << s.str();
	} catch(std::exception &e)
	{
	}
}
