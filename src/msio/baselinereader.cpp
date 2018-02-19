#include "baselinereader.h"

#include <set>
#include <stdexcept>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <casacore/tables/DataMan/IncrStManAccessor.h>
#include <casacore/tables/DataMan/StandardStManAccessor.h>
#include <casacore/tables/DataMan/TiledStManAccessor.h>
#include <casacore/tables/Tables/TableIter.h>
#include <casacore/tables/TaQL/ExprNode.h>

#include "../structures/timefrequencydata.h"

#include "../util/logger.h"

BaselineReader::BaselineReader(const std::string &msFile)
	: _measurementSet(msFile), _table(0), _dataColumnName("DATA"), _subtractModel(false), _readData(true), _readFlags(true),
	_polarizations()
{
	try {
		_table = new casacore::MeasurementSet(_measurementSet.Path(), casacore::MeasurementSet::Update);
	} catch(std::exception &e)
	{
		Logger::Warn << "Read-write opening of file " << msFile << " failed, trying read-only...\n";
		_table = new casacore::MeasurementSet(_measurementSet.Path());
		Logger::Warn << "Table opened in read-only: writing not possible.\n";
	}
}

BaselineReader::~BaselineReader()
{
	delete _table;
}

void BaselineReader::initObservationTimes()
{
	if(_observationTimes.size() == 0)
	{
		Logger::Debug << "Initializing observation times...\n";
		size_t sequenceCount = _measurementSet.SequenceCount();
		_observationTimes.resize(sequenceCount);
		for(size_t sequenceId=0; sequenceId!=sequenceCount; ++sequenceId)
		{
			const std::set<double> &times = _measurementSet.GetObservationTimesSet(sequenceId);
			unsigned index = 0;
			for(std::set<double>::const_iterator i=times.begin();i!=times.end();++i)
			{
				_observationTimes[sequenceId].insert(std::pair<double,size_t>(*i, index));
				_observationTimesVector.push_back(*i);
				++index;
			}
		}
	}
}

void BaselineReader::AddReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow, size_t sequenceId)
{
	initObservationTimes();
	
	addReadRequest(antenna1, antenna2, spectralWindow, sequenceId, 0, _observationTimes[sequenceId].size());
}

TimeFrequencyData BaselineReader::GetNextResult(std::vector<class UVW>& uvw)
{
	size_t requestIndex = 0;
	TimeFrequencyData data;
	data = TimeFrequencyData(
		_polarizations.data(),
		_polarizations.size(),
		_results[requestIndex]._realImages.data(),
		_results[requestIndex]._imaginaryImages.data());
	data.SetIndividualPolarizationMasks(_results[requestIndex]._flags.data());
	uvw = _results[0]._uvw;
	
	_results.erase(_results.begin() + requestIndex);

	return data;
}

void BaselineReader::initializePolarizations()
{
	if(_polarizations.empty())
	{
		casacore::MeasurementSet ms(_measurementSet.Path());
		
		casacore::MSDataDescription ddTable = ms.dataDescription();
		if(ddTable.nrow() == 0)
			throw std::runtime_error("DataDescription table is empty");
		casacore::ROScalarColumn<int> polIdColumn(ddTable, casacore::MSDataDescription::columnName(casacore::MSDataDescription::POLARIZATION_ID));
		int polarizationId = polIdColumn(0);
		for(size_t row=0; row!=ddTable.nrow(); ++row)
		{
			if(polIdColumn(row) != polarizationId)
				throw std::runtime_error("This measurement set has different polarizations listed in the datadescription table. This is non-standard, and AOFlagger cannot handle it.");
		}
		
		casacore::Table polTable = ms.polarization();
		casacore::ROArrayColumn<int> corTypeColumn(polTable, "CORR_TYPE"); 
		casacore::Array<int> corType = corTypeColumn(polarizationId);
		casacore::Array<int>::iterator iterend(corType.end());
		for (casacore::Array<int>::iterator iter=corType.begin(); iter!=iterend; ++iter)
		{
			PolarizationEnum polarization = Polarization::AipsIndexToEnum(*iter);
			_polarizations.push_back(polarization);
		}
	}
}

uint64_t BaselineReader::MeasurementSetDataSize(const string& filename)
{
	casacore::MeasurementSet ms(filename);
	
	casacore::MSSpectralWindow spwTable = ms.spectralWindow();
	
	casacore::ROScalarColumn<int> numChanCol(spwTable, casacore::MSSpectralWindow::columnName(casacore::MSSpectralWindowEnums::NUM_CHAN));
	size_t channelCount = numChanCol.get(0);
	if(channelCount == 0) throw std::runtime_error("No channels in set");
	if(ms.nrow() == 0) throw std::runtime_error("Table has no rows (no data)");
	
	typedef float num_t;
	typedef std::complex<num_t> complex_t;
	casacore::ROScalarColumn<int> ant1Column(ms, ms.columnName(casacore::MSMainEnums::ANTENNA1));
	casacore::ROScalarColumn<int> ant2Column(ms, ms.columnName(casacore::MSMainEnums::ANTENNA2));
	casacore::ROArrayColumn<complex_t> dataColumn(ms, ms.columnName(casacore::MSMainEnums::DATA));
	
	casacore::IPosition dataShape = dataColumn.shape(0);
	unsigned polarizationCount = dataShape[0];
	
	return
		(uint64_t) polarizationCount * (uint64_t) channelCount *
		(uint64_t) ms.nrow() * (uint64_t) (sizeof(num_t) * 2 + sizeof(bool));
}
